#include <chrono>
#include <fstream>
#include <aff3ct.hpp>
#include <streampu.hpp>

#include "Factory/DVBS2/DVBS2.hpp"
#ifdef DVBS2_LINK_UHD
#include "Module/Radio/Radio_USRP/Radio_USRP.hpp"
#endif

using namespace aff3ct;
using namespace aff3ct::module;

#define MULTI_THREADED // comment this line to disable multi-threaded RX

// global parameters
constexpr bool enable_logs = false;
#ifdef MULTI_THREADED
constexpr bool thread_pinnig = true;
constexpr bool active_waiting = false;
#endif /* MULTI_THREADED */

// aliases
template<class T> using uptr = std::unique_ptr<T>;

int main(int argc, char** argv)
{
#ifdef MULTI_THREADED
	if (thread_pinnig)
	{
		spu::tools::Thread_pinning::init();
		// spu::tools::Thread_pinning::set_logs(enable_logs);
		spu::tools::Thread_pinning::pin(0);
	}
#endif /* MULTI_THREADED */

	// setup signal handlers
	spu::tools::Signal_handler::init();

	// get the parameter to configure the tools and modules
	auto params = factory::DVBS2(argc, argv);

	std::map<std::string,tools::header_list> headers;
	std::vector<factory::Factory*> param_vec;
	param_vec.push_back(&params);
	tools::Header::print_parameters(param_vec, true, std::cout);

	// construct tools
	uptr<tools::Constellation<float>> cstl(new tools::Constellation_user<float>(params.constellation_file));
	uptr<tools::Interleaver_core<>> itl_core(factory::DVBS2::build_itl_core<>(params));
	tools::BCH_polynomial_generator<> poly_gen(params.N_bch_unshortened, 12, params.bch_prim_poly);

	// construct modules
	uptr<spu::module::Source<>      > source       (factory::DVBS2::build_source                  <>(params            ));
	uptr<spu::module::Sink<>        > sink         (factory::DVBS2::build_sink                    <>(params            ));
	uptr<Radio<>                    > radio        (factory::DVBS2::build_radio                   <>(params            ));
	uptr<Scrambler<>                > bb_scrambler (factory::DVBS2::build_bb_scrambler            <>(params            ));
	uptr<Decoder_HIHO<>             > BCH_decoder  (factory::DVBS2::build_bch_decoder             <>(params, poly_gen  ));
	uptr<tools::Codec_SIHO<>        > LDPC_cdc     (factory::DVBS2::build_ldpc_cdc                <>(params            ));
	uptr<Interleaver<float,uint32_t>> itl_rx       (factory::DVBS2::build_itl<float,uint32_t>       (params, *itl_core ));
	uptr<Modem<>                    > modem        (factory::DVBS2::build_modem                   <>(params, cstl.get()));
	uptr<Multiplier_sine_ccc_naive<>> freq_shift   (factory::DVBS2::build_freq_shift              <>(params            ));
	uptr<Synchronizer_frame<>       > sync_frame   (factory::DVBS2::build_synchronizer_frame      <>(params            ));
	uptr<Feedbacker<>               > feedbr       (factory::DVBS2::build_feedbacker              <>(params            ));
	uptr<Synchronizer_freq_fine<>   > sync_fine_lr (factory::DVBS2::build_synchronizer_lr         <>(params            ));
	uptr<Synchronizer_freq_fine<>   > sync_fine_pf (factory::DVBS2::build_synchronizer_freq_phase <>(params            ));
	uptr<Framer<>                   > framer       (factory::DVBS2::build_framer                  <>(params            ));
	uptr<Scrambler<float>           > pl_scrambler (factory::DVBS2::build_pl_scrambler            <>(params            ));
	uptr<Monitor_BFER<>             > monitor      (factory::DVBS2::build_monitor                 <>(params            ));
	uptr<Filter_RRC_ccr_naive<>     > matched_flt  (factory::DVBS2::build_matched_filter          <>(params            ));
	uptr<Synchronizer_timing <>     > sync_timing  (factory::DVBS2::build_synchronizer_timing     <>(params            ));
	uptr<Multiplier_AGC_cc_naive<>  > front_agc    (factory::DVBS2::build_channel_agc             <>(params            ));
	uptr<Multiplier_AGC_cc_naive<>  > mult_agc     (factory::DVBS2::build_agc_shift               <>(params            ));
	uptr<Estimator<>                > estimator    (factory::DVBS2::build_estimator               <>(params            ));
	uptr<Synchronizer_freq_coarse<> > sync_coarse_f(factory::DVBS2::build_synchronizer_freq_coarse<>(params            ));
	uptr<Synchronizer_step_mf_cc<>  > sync_step_mf (factory::DVBS2::build_synchronizer_step_mf_cc <>(params,
	                                                                                                 sync_coarse_f.get(),
	                                                                                                 matched_flt  .get(),
	                                                                                                 sync_timing  .get()));
	auto* LDPC_decoder = &LDPC_cdc->get_decoder_siho();

	monitor->disable_is_done(true); // this prevents the monitor to stop the sequence or pipeline with the `is_done()`
	                                // interface

	const size_t probe_buff = 200;
	// create reporters and probes for the statistics file
	spu::tools::Reporter_probe rep_fra_stats("Counter");
	spu::module::Probe_occurrence prb_fra_id("FRAME");
	spu::module::Probe_stream prb_fra_sid("STREAM");
	prb_fra_sid.set_custom_name("Probe<STREA>");
	rep_fra_stats.register_probes({ &prb_fra_id, &prb_fra_sid });
	rep_fra_stats.set_cols_buff_size(probe_buff);
	rep_fra_stats.set_n_frames(params.n_frames);

	spu::tools::Reporter_probe rep_rad_stats("Radio");
	spu::module::Probe_value<int32_t> prb_rad_ovf(1, "OVF");
	spu::module::Probe_value<int32_t> prb_rad_seq(1, "SEQ");
	rep_rad_stats.register_probes({ &prb_rad_ovf, &prb_rad_seq });
	prb_rad_ovf.set_col_unit("FLAG");
	prb_rad_seq.set_col_unit("ERR");
	rep_rad_stats.set_cols_buff_size(probe_buff);
	rep_rad_stats.set_n_frames(params.n_frames);

	spu::tools::Reporter_probe rep_sfm_stats("Frame Synchronization");
	spu::module::Probe_value<int32_t> prb_sfm_del(1, "DEL");
	spu::module::Probe_value<int32_t> prb_sfm_flg(1, "FLG");
	spu::module::Probe_value<float  > prb_sfm_tri(1, "TRI");
	rep_sfm_stats.register_probes({ &prb_sfm_del, &prb_sfm_flg, &prb_sfm_tri });
	prb_sfm_tri.set_col_fmtflags(std::ios_base::dec | std::ios_base::fixed);
	rep_sfm_stats.set_cols_buff_size(probe_buff);
	rep_sfm_stats.set_n_frames(params.n_frames);

	spu::tools::Reporter_probe rep_stm_stats("Timing Synchronization", "Gardner Algorithm");
	spu::module::Probe_value<int32_t> prb_stm_uff(1, "UFW");
	spu::module::Probe_value<float  > prb_stm_del(1, "DEL");
	rep_stm_stats.register_probes({ &prb_stm_uff, &prb_stm_del });
	prb_stm_uff.set_col_unit("FLAG");
	prb_stm_del.set_col_unit("FRAC");
	rep_stm_stats.set_cols_buff_size(probe_buff);
	rep_stm_stats.set_n_frames(params.n_frames);

	spu::tools::Reporter_probe rep_frq_stats("Frequency Synchronization");
	spu::module::Probe_value<float> prb_frq_coa(1, "COA");
	spu::module::Probe_value<float> prb_frq_lr (1, "L&R");
	spu::module::Probe_value<float> prb_frq_fin(1, "FIN");
	rep_frq_stats.register_probes({ &prb_frq_coa, &prb_frq_lr, &prb_frq_fin });
	rep_frq_stats.set_cols_unit("CFO");
	rep_frq_stats.set_cols_buff_size(probe_buff);
	rep_frq_stats.set_n_frames(params.n_frames);

	spu::tools::Reporter_probe rep_decstat_stats("Decoders Decoding Status", "('1' = success, '0' = fail)");
	spu::module::Probe_value<int8_t> prb_decstat_ldpc(1, "LDPC");
	spu::module::Probe_value<int8_t> prb_decstat_bch(1, "BCH");
	rep_decstat_stats.register_probes({ &prb_decstat_ldpc, &prb_decstat_bch });
	rep_decstat_stats.set_cols_buff_size(probe_buff);
	rep_decstat_stats.set_n_frames(params.n_frames);

	spu::tools::Reporter_probe rep_noise_stats("Signal Noise Ratio", "(SNR)");
	spu::module::Probe_value<float> prb_noise_sig(1, "SIGMA");
	spu::module::Probe_value<float> prb_noise_es(1, "Es/N0");
	spu::module::Probe_value<float> prb_noise_eb(1, "Eb/N0");
	rep_noise_stats.register_probes({ &prb_noise_sig, &prb_noise_es, &prb_noise_eb });
	prb_noise_sig.set_col_prec(4);
	prb_noise_es.set_col_unit("(dB)");
	prb_noise_eb.set_col_unit("(dB)");
	rep_noise_stats.set_cols_fmtflags(std::ios_base::dec | std::ios_base::fixed);
	rep_noise_stats.set_cols_buff_size(probe_buff);
	rep_noise_stats.set_n_frames(params.n_frames);

	spu::tools::Reporter_probe rep_BFER_stats("Bit Error Rate (BER)", "and Frame Error Rate (FER)");
	spu::module::Probe_value<int32_t> prb_bfer_be(1, "BE");
	spu::module::Probe_value<int32_t> prb_bfer_fe(1, "FE");
	spu::module::Probe_value<float> prb_bfer_ber(1, "BER");
	spu::module::Probe_value<float> prb_bfer_fer(1, "FER");
	rep_BFER_stats.register_probes({ &prb_bfer_be, &prb_bfer_fe, &prb_bfer_ber, &prb_bfer_fer });
	rep_BFER_stats.set_cols_buff_size(probe_buff);
	rep_BFER_stats.set_n_frames(params.n_frames);

	spu::tools::Reporter_probe rep_thr_stats("Throughput", "and elapsed time");
	spu::module::Probe_throughput prb_thr_thr(params.K_bch, "THR");
	spu::module::Probe_value<double> prb_thr_the(1, "TTHR");
	spu::module::Probe_latency prb_thr_lat("LAT");
	spu::module::Probe_time prb_thr_time("TIME");
	spu::module::Probe_timestamp prb_thr_tsta("TSTA");
	rep_thr_stats.register_probes({ &prb_thr_thr, &prb_thr_the, &prb_thr_lat, &prb_thr_time, &prb_thr_tsta });
	prb_thr_thr.set_col_unit("(Mbps)");
	prb_thr_the.set_col_unit("Theory");
	prb_thr_the.set_col_fmtflags(std::ios_base::dec | std::ios_base::fixed);
	prb_thr_tsta.set_col_size(15);
	rep_thr_stats.set_cols_buff_size(probe_buff);
	rep_thr_stats.set_n_frames(params.n_frames);

	spu::tools::Terminal_dump terminal_stats({ &rep_fra_stats, &rep_rad_stats,     &rep_sfm_stats,   &rep_stm_stats,
	                                           &rep_frq_stats, &rep_decstat_stats, &rep_noise_stats, &rep_BFER_stats,
	                                           &rep_thr_stats });

	// add custom name to some modules
	LDPC_decoder ->set_custom_name("LDPC Decoder");
	BCH_decoder  ->set_custom_name("BCH Decoder" );
	sync_fine_lr ->set_custom_name("L&R F Syn"   );
	sync_fine_pf ->set_custom_name("Fine P/F Syn");
	sync_timing  ->set_custom_name("Gardner Syn" );
	sync_frame   ->set_custom_name("Frame Syn"   );
	matched_flt  ->set_custom_name("Matched Flt" );
	sync_coarse_f->set_custom_name("Coarse_Synch");
	sync_step_mf ->set_custom_name("MF Synch"    );
	mult_agc     ->set_custom_name("Mult agc"    );

	std::vector<double> theoretical_thr(params.n_frames, params.p_rad.rx_rate/1e6 * (double)params.K_bch /
	                                                     ((double)params.pl_frame_size * (double)params.p_shp.osf));

	// the full transmission chain binding
	(*front_agc    )[             mlt::sck::imultiply    ::X_N     ] = (*radio        )[             rad::sck::receive      ::Y_N1    ];
	(*sync_coarse_f)[             sfc::sck::synchronize  ::X_N1    ] = (*front_agc    )[             mlt::sck::imultiply    ::Z_N     ];
	(*matched_flt  )[             flt::sck::filter1      ::X_N1    ] = (*sync_coarse_f)[             sfc::sck::synchronize  ::Y_N2    ];
	(*matched_flt  )[             flt::sck::filter2      ::X_N1    ] = (*sync_coarse_f)[             sfc::sck::synchronize  ::Y_N2    ];
	(*matched_flt  )[             flt::sck::filter2      ::Y_N2h   ] = (*matched_flt  )[             flt::sck::filter1      ::Y_N2    ];
	(*sync_timing  )[             stm::sck::synchronize  ::X_N1    ] = (*matched_flt  )[             flt::sck::filter2      ::Y_N2    ];
	(*sync_timing  )[             stm::sck::extract      ::B_N1    ] = (*sync_timing  )[             stm::sck::synchronize  ::B_N1    ];
	(*sync_timing  )[             stm::sck::extract      ::Y_N1    ] = (*sync_timing  )[             stm::sck::synchronize  ::Y_N1    ];
	(*mult_agc     )[             mlt::sck::imultiply    ::X_N     ] = (*sync_timing  )[             stm::sck::extract      ::Y_N2    ];
	(*sync_frame   )[             sfm::sck::synchronize1 ::X_N1    ] = (*mult_agc     )[             mlt::sck::imultiply    ::Z_N     ];
	(*sync_frame   )[             sfm::sck::synchronize2 ::X_N1    ] = (*mult_agc     )[             mlt::sck::imultiply    ::Z_N     ];
	(*sync_frame   )[             sfm::sck::synchronize2 ::cor_SOF ] = (*sync_frame   )[             sfm::sck::synchronize1 ::cor_SOF ];
	(*sync_frame   )[             sfm::sck::synchronize2 ::cor_PLSC] = (*sync_frame   )[             sfm::sck::synchronize1 ::cor_PLSC];
	(*pl_scrambler )[             scr::sck::descramble   ::Y_N1    ] = (*sync_frame   )[             sfm::sck::synchronize2 ::Y_N2    ];
	(*sync_fine_lr )[             sff::sck::synchronize  ::X_N1    ] = (*pl_scrambler )[             scr::sck::descramble   ::Y_N2    ];
	(*sync_fine_pf )[             sff::sck::synchronize  ::X_N1    ] = (*sync_fine_lr )[             sff::sck::synchronize  ::Y_N2    ];
	(*framer       )[             frm::sck::remove_plh   ::Y_N1    ] = (*sync_fine_pf )[             sff::sck::synchronize  ::Y_N2    ];
	(*estimator    )[             est::sck::estimate     ::X_N     ] = (*framer       )[             frm::sck::remove_plh   ::Y_N2    ];
	(*modem        )[             mdm::sck::demodulate   ::CP      ] = (*estimator    )[             est::sck::estimate     ::SIG     ];
	(*modem        )[             mdm::sck::demodulate   ::Y_N1    ] = (*framer       )[             frm::sck::remove_plh   ::Y_N2    ];
	(*itl_rx       )[             itl::sck::deinterleave ::itl     ] = (*modem        )[             mdm::sck::demodulate   ::Y_N2    ];
	(*LDPC_decoder )[             dec::sck::decode_siho  ::Y_N     ] = (*itl_rx       )[             itl::sck::deinterleave ::nat     ];
	(*BCH_decoder  )[             dec::sck::decode_hiho  ::Y_N     ] = (*LDPC_decoder )[             dec::sck::decode_siho  ::V_K     ];
	(*bb_scrambler )[             scr::sck::descramble   ::Y_N1    ] = (*BCH_decoder  )[             dec::sck::decode_hiho  ::V_K     ];
	(*monitor      )[             mnt::sck::check_errors2::U       ] = (*source       )[spu::module::src::sck::generate     ::out_data];
	(*monitor      )[             mnt::sck::check_errors2::V       ] = (*bb_scrambler )[             scr::sck::descramble   ::Y_N2    ];
	(*sink         )[spu::module::snk::sck::send         ::in_data ] = (*bb_scrambler )[             scr::sck::descramble   ::Y_N2    ];
	// bind the probes
	prb_thr_the     [spu::module::prb::sck::probe        ::in      ] = theoretical_thr.data();
	prb_rad_ovf     [spu::module::prb::sck::probe        ::in      ] = (*radio        )[             rad::sck::receive      ::OVF     ];
	prb_rad_seq     [spu::module::prb::sck::probe        ::in      ] = (*radio        )[             rad::sck::receive      ::SEQ     ];
	prb_frq_coa     [spu::module::prb::sck::probe        ::in      ] = (*sync_coarse_f)[             sfc::sck::synchronize  ::FRQ     ];
	prb_stm_del     [spu::module::prb::sck::probe        ::in      ] = (*sync_timing  )[             stm::sck::synchronize  ::MU      ];
	prb_stm_uff     [spu::module::prb::sck::probe        ::in      ] = (*sync_timing  )[             stm::sck::extract      ::UFW     ];
	prb_sfm_del     [spu::module::prb::sck::probe        ::in      ] = (*sync_frame   )[             sfm::sck::synchronize2 ::DEL     ];
	prb_sfm_tri     [spu::module::prb::sck::probe        ::in      ] = (*sync_frame   )[             sfm::sck::synchronize2 ::TRI     ];
	prb_sfm_flg     [spu::module::prb::sck::probe        ::in      ] = (*sync_frame   )[             sfm::sck::synchronize2 ::FLG     ];
	prb_frq_lr      [spu::module::prb::sck::probe        ::in      ] = (*sync_fine_lr )[             sff::sck::synchronize  ::FRQ     ];
	prb_frq_fin     [spu::module::prb::sck::probe        ::in      ] = (*sync_fine_pf )[             sff::sck::synchronize  ::FRQ     ];
	prb_noise_es    [spu::module::prb::sck::probe        ::in      ] = (*estimator    )[             est::sck::estimate     ::Es_N0   ];
	prb_noise_eb    [spu::module::prb::sck::probe        ::in      ] = (*estimator    )[             est::sck::estimate     ::Eb_N0   ];
	prb_noise_sig   [spu::module::prb::sck::probe        ::in      ] = (*estimator    )[             est::sck::estimate     ::SIG     ];
	prb_decstat_ldpc[spu::module::prb::sck::probe        ::in      ] = (*LDPC_decoder )[             dec::sck::decode_siho  ::CWD     ];
	prb_decstat_bch [spu::module::prb::sck::probe        ::in      ] = (*BCH_decoder  )[             dec::sck::decode_hiho  ::CWD     ];
	prb_thr_thr     [spu::module::prb::tsk::probe                  ] = (*bb_scrambler )[             scr::sck::descramble   ::Y_N2    ];
	prb_thr_lat     [spu::module::prb::tsk::probe                  ] = (*sink         )[spu::module::snk::sck::send         ::status  ];
	prb_thr_time    [spu::module::prb::tsk::probe                  ] = (*sink         )[spu::module::snk::sck::send         ::status  ];
	prb_thr_tsta    [spu::module::prb::tsk::probe                  ] = (*sink         )[spu::module::snk::sck::send         ::status  ];
	prb_bfer_be     [spu::module::prb::sck::probe        ::in      ] = (*monitor      )[             mnt::sck::check_errors2::BE      ];
	prb_bfer_fe     [spu::module::prb::sck::probe        ::in      ] = (*monitor      )[             mnt::sck::check_errors2::FE      ];
	prb_bfer_ber    [spu::module::prb::sck::probe        ::in      ] = (*monitor      )[             mnt::sck::check_errors2::BER     ];
	prb_bfer_fer    [spu::module::prb::sck::probe        ::in      ] = (*monitor      )[             mnt::sck::check_errors2::FER     ];
	prb_fra_id      [spu::module::prb::tsk::probe                  ] = (*sink         )[spu::module::snk::sck::send         ::status  ];
	prb_fra_sid     [spu::module::prb::tsk::probe                  ] = (*sink         )[spu::module::snk::sck::send         ::status  ];

	// first stages of the whole transmission sequence
	const std::vector<spu::runtime::Task*> firsts_t = { &(*radio)[rad::tsk::receive],
	                                                    &(*source)[spu::module::src::tsk::generate],
	                                                    &prb_thr_the[spu::module::prb::tsk::probe] };

#ifdef MULTI_THREADED
	auto start_clone = std::chrono::system_clock::now();
	std::cout << "Cloning the modules of the parallel sequence... ";
	std::cout.flush();

	// pipeline definition with separation stages
	const std::vector<std::tuple<std::vector<spu::runtime::Task*>,
	                             std::vector<spu::runtime::Task*>,
	                             std::vector<spu::runtime::Task*>>> sep_stages =
	{ // pipeline stage 0
	  std::make_tuple<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*radio)[rad::tsk::receive], &(*source)[spu::module::src::tsk::generate],
	      &prb_rad_ovf[spu::module::prb::tsk::probe], &prb_rad_seq[spu::module::prb::tsk::probe],
	      &prb_thr_the[spu::module::prb::tsk::probe] },
	    { &(*radio)[rad::tsk::receive], &(*source)[spu::module::src::tsk::generate] },
	    { /* no exclusions in this stage */ } ),
	  // pipeline stage 1
	  std::make_tuple<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*front_agc)[mlt::tsk::imultiply] },
	    { &prb_frq_coa[spu::module::prb::tsk::probe] },
	    { &(*matched_flt)[flt::tsk::filter1] } ),
	  // pipeline stage 2
	  std::make_tuple<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*matched_flt)[flt::tsk::filter1] },
	    { &(*matched_flt)[flt::tsk::filter1] },
	    { /* no exclusions in this stage */ } ),
	  // pipeline stage 3
	  std::make_tuple<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*matched_flt)[flt::tsk::filter2] },
	    { &(*matched_flt)[flt::tsk::filter2] },
	    { /* no exclusions in this stage */ } ),
	  // pipeline stage 4
	  std::make_tuple<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*sync_timing)[stm::tsk::synchronize], &prb_stm_del[spu::module::prb::tsk::probe] },
	    { &(*sync_timing)[stm::tsk::synchronize] },
	    { /* no exclusions in this stage */ } ),
	  // pipeline stage 5
	  std::make_tuple<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*sync_timing)[stm::tsk::extract] },
	    { &(*mult_agc   )[mlt::tsk::imultiply] },
	    { /* no exclusions in this stage */ } ),
	  // pipeline stage 6
	  std::make_tuple<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*sync_frame)[sfm::tsk::synchronize1] },
	    { &(*sync_frame)[sfm::tsk::synchronize1] },
	    { /* no exclusions in this stage */ } ),
	  // pipeline stage 7
	  std::make_tuple<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*sync_frame)[sfm::tsk::synchronize2],
	      &prb_sfm_del[spu::module::prb::tsk::probe], &prb_sfm_tri[spu::module::prb::tsk::probe],
          &prb_sfm_flg[spu::module::prb::tsk::probe] },
	    { &(*sync_frame)[sfm::tsk::synchronize2] },
	    { /* no exclusions in this stage */ } ),
	  // pipeline stage 8
	  std::make_tuple<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*pl_scrambler)[scr::tsk::descramble], &prb_frq_lr[spu::module::prb::tsk::probe] },
	    { &(*sync_fine_lr)[sff::tsk::synchronize] },
	    { /* no exclusions in this stage */ } ),
	  // pipeline stage 9
	  std::make_tuple<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*sync_fine_pf)[sff::tsk::synchronize], &prb_frq_fin[spu::module::prb::tsk::probe]},
	    { &(*sync_fine_pf)[sff::tsk::synchronize] },
	    { /* no exclusions in this stage */ } ),
	  // pipeline stage 10
	  std::make_tuple<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*framer)[frm::tsk::remove_plh], &prb_noise_sig[spu::module::prb::tsk::probe],
          &prb_noise_es[spu::module::prb::tsk::probe],
	      &prb_noise_eb[spu::module::prb::tsk::probe] },
	    { &(*estimator)[est::tsk::estimate] },
	    { &(*modem)[mdm::tsk::demodulate] } ),
	  // pipeline stage 11
	  std::make_tuple<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*modem)[mdm::tsk::demodulate] },
	    { &(*bb_scrambler)[scr::tsk::descramble] },
	    { &prb_decstat_ldpc[spu::module::prb::tsk::probe], &prb_decstat_bch[spu::module::prb::tsk::probe],
	      &prb_thr_thr[spu::module::prb::tsk::probe] } ),
	  // pipeline stage 12
	  std::make_tuple<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*monitor)[mnt::tsk::check_errors2], &(*sink)[spu::module::snk::tsk::send],
          &prb_decstat_ldpc[spu::module::prb::tsk::probe],
	      &prb_decstat_bch[spu::module::prb::tsk::probe], &prb_thr_thr[spu::module::prb::tsk::probe] },
	    { /* end of the sequence */ },
	    { /* no exclusions in this stage */ } ),
	};
	// number of threads per stages
	const std::vector<size_t> n_threads_per_stages = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 40, 1 };
	// synchronization buffer size between stages
	const std::vector<size_t> buffer_sizes(sep_stages.size() -1, 1);
	// type of waiting between stages (true = active, false = passive)
	const std::vector<bool> active_waitings(sep_stages.size() -1, active_waiting);
	// enable thread pinning
	const std::vector<bool> thread_pinnigs(sep_stages.size(), thread_pinnig);
	// process unit (pu) ids per stage for thread pinning
	const std::vector<std::vector<size_t>> puids = { {  2*4 },                                         // for stage  0
	                                                 {  3*4 },                                         // for stage  1
	                                                 {  4*4 },                                         // for stage  2
	                                                 {  5*4 },                                         // for stage  3
	                                                 {  6*4 },                                         // for stage  4
	                                                 {  7*4 },                                         // for stage  5
	                                                 {  8*4 },                                         // for stage  6
	                                                 {  9*4 },                                         // for stage  7
	                                                 { 10*4 },                                         // for stage  8
	                                                 { 11*4 },                                         // for stage  9
	                                                 { 12*4 },                                         // for stage 10
	                                                 { 14*4, 28*4, 29*4, 30*4, 31*4, 32*4, 33*4, 34*4, // for stage 11
	                                                   35*4, 36*4, 37*4, 38*4, 39*4, 15*4, 40*4, 16*4,
	                                                   41*4, 17*4, 42*4, 18*4, 43*4, 19*4, 44*4, 20*4,
	                                                   45*4, 21*4, 46*4, 22*4, 47*4, 23*4, 48*4, 24*4,
	                                                   49*4, 25*4, 50*4, 26*4, 51*4, 27*4, 52*4, 53*4,
	                                                   54*4, 55*4},
	                                                 { 13*4 } };                                       // for stage 12

	spu::runtime::Pipeline pipeline_transmission(firsts_t, sep_stages, n_threads_per_stages, buffer_sizes,
	                                             active_waitings, thread_pinnigs, puids);

	if (enable_logs)
	{
		std::ofstream f("rx_pipeline_transmission.dot");
		pipeline_transmission.export_dot(f);
	}

	auto tasks_per_types = pipeline_transmission.get_tasks_per_types();
	pipeline_transmission.unbind_adaptors();
	auto end_clone = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds_clone = end_clone - start_clone;
	std::cout << "Done (" << elapsed_seconds_clone.count() << "s)." << std::endl;

	if (thread_pinnig)
		spu::tools::Thread_pinning::pin(0);
#else
	spu::runtime::Sequence sequence_transmission(firsts_t);
	if (enable_logs)
	{
		std::ofstream f("rx_sequence_transmission.dot");
		sequence_transmission.export_dot(f);
	}
	auto tasks_per_types = sequence_transmission.get_tasks_per_types();
#endif /* MULTI_THREADED */

	// configuration of the sequence tasks
	for (auto& type : tasks_per_types) for (auto& tsk : type)
	{
		tsk->set_autoalloc      (true              ); // enable the automatic allocation of the data in the tasks
		tsk->set_debug          (params.debug      ); // disable the debug mode
		tsk->set_debug_limit    (params.debug_limit); // display only the 16 first bits if the debug mode is enabled
		tsk->set_debug_precision(8                 );
		tsk->set_stats          (params.stats      ); // enable the statistics

		// enable the fast mode (= disable the useless verifs in the tasks) if there is no debug and stats modes
		if (!tsk->is_debug() && !tsk->is_stats())
			tsk->set_fast(true);
	}

#ifdef DVBS2_LINK_UHD
	const int radio_flush_period = params.n_frames * 100;
	auto radio_usrp = dynamic_cast<Radio_USRP<>*>(radio.get());
#endif

	std::ofstream stats_file("stats.txt");
	if (!params.no_wl_phases) {
		// ============================================================================================================
		// WAITING PHASE ==============================================================================================
		// ============================================================================================================
		auto start_waiting = std::chrono::system_clock::now();
		std::cout << "Waiting phase... ";
		std::cout.flush();

		// partial unbinding
		(*sync_coarse_f)[             sfc::sck::synchronize::X_N1].unbind((*front_agc    )[mlt::sck::imultiply  ::Z_N ]);
		(*sync_timing  )[             stm::sck::extract    ::B_N1].unbind((*sync_timing  )[stm::sck::synchronize::B_N1]);
		(*sync_timing  )[             stm::sck::extract    ::Y_N1].unbind((*sync_timing  )[stm::sck::synchronize::Y_N1]);
		prb_frq_coa     [spu::module::prb::sck::probe      ::in  ].unbind((*sync_coarse_f)[sfc::sck::synchronize::FRQ ]);
		prb_stm_del     [spu::module::prb::sck::probe      ::in  ].unbind((*sync_timing  )[stm::sck::synchronize::MU  ]);

		// partial binding
		(*sync_step_mf)[             smf::sck::synchronize::X_N1] = (*front_agc   )[mlt::sck::imultiply   ::Z_N ];
		(*sync_step_mf)[             smf::sck::synchronize::DEL ] = (*feedbr      )[fbr::sck::produce     ::Y_N ];
		(*feedbr      )[             fbr::sck::memorize   ::X_N ] = (*sync_frame  )[sfm::sck::synchronize2::DEL ];
		(*sync_timing )[             stm::sck::extract    ::B_N1] = (*sync_step_mf)[smf::sck::synchronize ::B_N1];
		(*sync_timing )[             stm::sck::extract    ::Y_N1] = (*sync_step_mf)[smf::sck::synchronize ::Y_N1];
		prb_frq_coa    [spu::module::prb::sck::probe      ::in  ] = (*sync_step_mf)[smf::sck::synchronize ::FRQ ];
		prb_stm_del    [spu::module::prb::sck::probe      ::in  ] = (*sync_step_mf)[smf::sck::synchronize ::MU  ];

		std::vector<spu::runtime::Task*> firsts_wl12 = { &(*radio    )[             rad::tsk::receive],
		                                                 &prb_sfm_del [spu::module::prb::tsk::probe  ],
		                                                 &prb_sfm_tri [spu::module::prb::tsk::probe  ],
		                                                 &prb_sfm_flg [spu::module::prb::tsk::probe  ],
		                                                 &prb_thr_lat [spu::module::prb::tsk::probe  ],
		                                                 &prb_thr_time[spu::module::prb::tsk::probe  ],
		                                                 &prb_thr_tsta[spu::module::prb::tsk::probe  ],
		                                                 &prb_fra_id  [spu::module::prb::tsk::probe  ],
		                                                 &prb_fra_sid [spu::module::prb::tsk::probe  ],
		                                                 &(*feedbr   )[             fbr::tsk::produce] };

		std::vector<spu::runtime::Task*> lasts_wl12 = { &(*feedbr)[fbr::tsk::memorize] };
		std::vector<spu::runtime::Task*> exclude_wl12 = { &(*pl_scrambler)[scr::tsk::descramble] };

		spu::runtime::Sequence sequence_waiting_and_learning_1_2(firsts_wl12, lasts_wl12, exclude_wl12);
		sequence_waiting_and_learning_1_2.set_auto_stop(false);

		if (enable_logs)
		{
			std::ofstream fs1("rx_sequence_waiting_and_learning_1_2.dot");
			sequence_waiting_and_learning_1_2.export_dot(fs1);
		}

		// display the legend in the terminal
		std::ofstream waiting_stats("waiting_stats.txt");
		waiting_stats << "#################" << std::endl;
		waiting_stats << "# WAITING PHASE #" << std::endl;
		waiting_stats << "#################" << std::endl;
		terminal_stats.legend(waiting_stats);

		sync_coarse_f->set_PLL_coeffs(1, 1/std::sqrt(2.0), 1e-4);
		prb_thr_thr .reset();
		prb_thr_lat .reset();
		prb_thr_time.reset();
		sequence_waiting_and_learning_1_2.exec([&](const std::vector<const int*>& statuses)
		{
			const auto m = prb_fra_id.get_occurrences();
			if (statuses.back() != nullptr)
				terminal_stats.temp_report(waiting_stats);
			else if (enable_logs)
				std::clog << rang::tag::warning << "Sequence aborted! (waiting phase, m = " << m << ")" << std::endl;
#ifdef DVBS2_LINK_UHD
			if (radio_usrp != nullptr && m % radio_flush_period == 0 && !sync_frame->get_packet_flag())
				radio_usrp->flush();
#endif
			return sync_frame->get_packet_flag();
		});

#ifdef DVBS2_LINK_UHD
		if (radio_usrp != nullptr)
			radio_usrp->flush();
#endif
		sync_step_mf->reset();
		sync_frame  ->reset();
		sync_timing ->reset();
		prb_fra_id   .reset();
		prb_fra_sid  .reset();

		auto end_waiting = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds_waiting = end_waiting - start_waiting;
		std::cout << "Done (" << elapsed_seconds_waiting.count() << "s)." << std::endl;

		// ============================================================================================================
		// LEARNING PHASE 1 & 2 =======================================================================================
		// ============================================================================================================
		auto start_learning = std::chrono::system_clock::now();
		std::cout << "Learning phase... ";
		std::cout.flush();

		// display the legend in the terminal
		stats_file << "####################" << std::endl;
		stats_file << "# LEARNING PHASE 1 #" << std::endl;
		stats_file << "####################" << std::endl;
		terminal_stats.legend(stats_file);

		int limit = 150;
		sync_coarse_f->set_PLL_coeffs(1, 1/std::sqrt(2.0), 1e-4);
		prb_thr_thr .reset();
		prb_thr_lat .reset();
		prb_thr_time.reset();
		sequence_waiting_and_learning_1_2.exec([&](const std::vector<const int*>& statuses)
		{
			const auto m = prb_fra_id.get_occurrences();
			if (statuses.back() != nullptr)
				terminal_stats.temp_report(stats_file);
			else if (enable_logs)
				std::clog << rang::tag::warning << "Sequence aborted! (learning phase 1&2, m = " << m << ")"
				          << std::endl;

			if (limit == 150 && m >= 150)
			{
				stats_file << "####################" << std::endl;
				stats_file << "# LEARNING PHASE 2 #" << std::endl;
				stats_file << "####################" << std::endl;
				terminal_stats.legend(stats_file);
				limit = m + 150;
				sync_coarse_f->set_PLL_coeffs(1, 1/std::sqrt(2.0), 5e-5);
			}
			return m >= limit;
		});

		// ============================================================================================================
		// LEARNING PHASE 3 ===========================================================================================
		// ============================================================================================================
		// partial unbinding
		(*sync_step_mf)[             smf::sck::synchronize::X_N1].unbind((*front_agc   )[mlt::sck::imultiply   ::Z_N ]);
		(*feedbr      )[             fbr::sck::memorize   ::X_N ].unbind((*sync_frame  )[sfm::sck::synchronize2::DEL ]);
		(*sync_timing )[             stm::sck::extract    ::B_N1].unbind((*sync_step_mf)[smf::sck::synchronize ::B_N1]);
		(*sync_timing )[             stm::sck::extract    ::Y_N1].unbind((*sync_step_mf)[smf::sck::synchronize ::Y_N1]);
		prb_frq_coa    [spu::module::prb::sck::probe      ::in  ].unbind((*sync_step_mf)[smf::sck::synchronize ::FRQ ]);
		prb_stm_del    [spu::module::prb::sck::probe      ::in  ].unbind((*sync_step_mf)[smf::sck::synchronize ::MU  ]);

		// partial binding
		(*sync_coarse_f)[             sfc::sck::synchronize::X_N1] = (*front_agc    )[mlt::sck::imultiply  ::Z_N ];
		(*sync_timing  )[             stm::sck::extract    ::B_N1] = (*sync_timing  )[stm::sck::synchronize::B_N1];
		(*sync_timing  )[             stm::sck::extract    ::Y_N1] = (*sync_timing  )[stm::sck::synchronize::Y_N1];
		prb_frq_coa     [spu::module::prb::sck::probe      ::in  ] = (*sync_coarse_f)[sfc::sck::synchronize::FRQ ];
		prb_stm_del     [spu::module::prb::sck::probe      ::in  ] = (*sync_timing  )[stm::sck::synchronize::MU  ];

		std::vector<spu::runtime::Task*> firsts_l3 = { &(*radio    )[             rad::tsk::receive],
		                                               &prb_thr_lat [spu::module::prb::tsk::probe  ],
		                                               &prb_thr_time[spu::module::prb::tsk::probe  ],
		                                               &prb_thr_tsta[spu::module::prb::tsk::probe  ],
		                                               &prb_fra_id  [spu::module::prb::tsk::probe  ],
		                                               &prb_fra_sid [spu::module::prb::tsk::probe  ],
		                                               &prb_frq_fin [spu::module::prb::tsk::probe  ] };

		std::vector<spu::runtime::Task*> lasts_l3 = { &(*sync_fine_pf)[sff::tsk::synchronize] };

		spu::runtime::Sequence sequence_learning_3(firsts_l3, lasts_l3);
		sequence_learning_3.set_auto_stop(false);

		if (enable_logs)
		{
			std::ofstream fs2("rx_sequence_learning_3.dot");
			sequence_learning_3.export_dot(fs2);
		}

		stats_file << "####################" << std::endl;
		stats_file << "# LEARNING PHASE 3 #" << std::endl;
		stats_file << "####################" << std::endl;
		terminal_stats.legend(stats_file);

		limit = prb_fra_id.get_occurrences() + 200;
		prb_thr_thr.reset();
		prb_thr_lat.reset();
		sequence_learning_3.exec([&](const std::vector<const int*>& statuses)
		{
			const auto m = prb_fra_id.get_occurrences();
			if (statuses.back() != nullptr)
				terminal_stats.temp_report(stats_file);
			else if (enable_logs)
				std::clog << rang::tag::warning << "Sequence aborted! (learning phase 3, m = " << m << ")" << std::endl;
			return m >= limit;
		});

		auto end_learning = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds_learning = end_learning - start_learning;
		std::cout << "Done (" << elapsed_seconds_learning.count() << "s)." << std::endl;
	}

	// ================================================================================================================
	// TRANSMISSION PHASE =============================================================================================
	// ================================================================================================================
	// allocate reporters to display results in the terminal
	tools::Sigma<> noise_est;
	tools::Reporter_noise<>      rep_noise( noise_est);
	tools::Reporter_BFER<>       rep_BFER (*monitor  );
	tools::Reporter_throughput<> rep_thr  (*monitor  );

	// allocate a terminal that will display the collected data from the reporters
	spu::tools::Terminal_std terminal({ &rep_noise, &rep_BFER, &rep_thr });

	// display the legend in the terminal
	terminal.legend();

	if (params.ter_freq != std::chrono::nanoseconds(0))
		terminal.start_temp_report(params.ter_freq);

	sync_timing->set_act(true);

	stats_file << "######################" << std::endl;
	stats_file << "# TRANSMISSION PHASE #" << std::endl;
	stats_file << "######################" << std::endl;
	terminal_stats.legend(stats_file);

	// reset the statistics of the tasks before the transmission phase
	for (auto& type : tasks_per_types)
		for (auto& tsk : type)
			tsk->reset();

	prb_thr_thr.reset();
	prb_thr_lat.reset();
#ifdef MULTI_THREADED
	pipeline_transmission.bind_adaptors();
	pipeline_transmission.exec({
		[] (const std::vector<const int*>& statuses) { return false; }, // stop condition stage 0
		[] (const std::vector<const int*>& statuses) { return false; }, // stop condition stage 1
		[] (const std::vector<const int*>& statuses) { return false; }, // stop condition stage 2
		[] (const std::vector<const int*>& statuses) { return false; }, // stop condition stage 3
		[] (const std::vector<const int*>& statuses) { return false; }, // stop condition stage 4
		[&prb_fra_id] (const std::vector<const int*>& statuses)         // stop condition stage 5
		{
			if (statuses.back() == nullptr && enable_logs)
				std::clog << std::endl << rang::tag::warning << "Sequence aborted! (transmission phase, stage = 3"
				          << ", m = " << prb_fra_id.get_occurrences() << ")" << std::endl;
			return false;
		},
		[] (const std::vector<const int*>& statuses) { return false; }, // stop condition stage 6
		[] (const std::vector<const int*>& statuses) { return false; }, // stop condition stage 7
		[] (const std::vector<const int*>& statuses) { return false; }, // stop condition stage 8
		[] (const std::vector<const int*>& statuses) { return false; }, // stop condition stage 9
		[&noise_est, &estimator] (const std::vector<const int*>& statuses) // stop condition stage 10
		{
			// update "noise_est" for the terminal display
			if (((float*)(*estimator)[est::sck::estimate::SIG].get_dataptr())[0] > 0)
				noise_est.set_values(((float*)(*estimator)[est::sck::estimate::SIG  ].get_dataptr())[0],
				                     ((float*)(*estimator)[est::sck::estimate::Eb_N0].get_dataptr())[0],
				                     ((float*)(*estimator)[est::sck::estimate::Es_N0].get_dataptr())[0]);
			return false;
		},
		[] (const std::vector<const int*>& statuses) { return false; }, // stop condition stage 11
		[&terminal_stats, &stats_file] (const std::vector<const int*>& statuses) // stop condition stage 12
		{
			terminal_stats.temp_report(stats_file);
			return false;
		}});
#else
	// start the transmission sequence
	sequence_transmission.exec([&prb_fra_id, &terminal_stats, &stats_file, &noise_est, &estimator]
		(const std::vector<const int*>& statuses)
		{
			if (statuses.back() != nullptr)
			{
				terminal_stats.temp_report(stats_file);
				// update "noise_est" for the terminal display
				if (((float*)(*estimator)[est::sck::estimate::SIG].get_dataptr())[0] > 0)
					noise_est.set_values(((float*)(*estimator)[est::sck::estimate::SIG  ].get_dataptr())[0],
					                     ((float*)(*estimator)[est::sck::estimate::Eb_N0].get_dataptr())[0],
					                     ((float*)(*estimator)[est::sck::estimate::Es_N0].get_dataptr())[0]);
			}
			else if (enable_logs)
				std::clog << std::endl << rang::tag::warning << "Sequence aborted! (transmission phase, m = "
				          << prb_fra_id.get_occurrences() << ")" << std::endl;
			return false;
		});
#endif /* MULTI_THREADED */

#ifdef DVBS2_LINK_UHD
	// stop the radio thread
	if (radio_usrp != nullptr)
		radio_usrp->cancel_waiting();
#endif

	// display the performance (BER and FER) in the terminals
	terminal      .final_report(          );
	terminal_stats.final_report(stats_file);

	if (params.stats)
	{
		std::ofstream file_stream;
		if (!params.stats_path.empty())
			file_stream.open(params.stats_path);
		std::ostream& stats_out = params.stats_path.empty() ? std::cout : file_stream;
		const auto ordered = true;
#ifdef MULTI_THREADED
		auto stages = pipeline_transmission.get_stages();
		for (size_t ss = 0; ss < stages.size(); ss++)
		{
			stats_out << "#" << std::endl << "# Sequence stage " << ss << " (" << stages[ss]->get_n_threads()
			          << " thread(s)): " << std::endl;
			spu::tools::Stats::show(stages[ss]->get_tasks_per_types(), ordered, true, stats_out);
		}
#else
		stats_out << "#" << std::endl << "# Sequence sequential (" << sequence_transmission.get_n_threads()
		          << " thread(s)): " << std::endl;
		spu::tools::Stats::show(sequence_transmission.get_tasks_per_types(), ordered, true, stats_out);
#endif /* MULTI_THREADED */
	}

	std::cout << "#" << std::endl;
	std::cout << "# End of the simulation" << std::endl;

#ifdef MULTI_THREADED
	if (thread_pinnig)
		spu::tools::Thread_pinning::destroy();
#endif /* MULTI_THREADED */

	return EXIT_SUCCESS;
}

#include <vector>
#include <string>
#include <chrono>
#include <numeric>
#include <iostream>
#include <algorithm>

#include <aff3ct.hpp>
#include <streampu.hpp>

#include "Factory/DVBS2/DVBS2.hpp"
#include "Tools/Reporter/Reporter_throughput_DVBS2.hpp"
#include "Tools/Reporter/Reporter_noise_DVBS2.hpp"

using namespace aff3ct;
using namespace aff3ct::module;

#define MULTI_THREADED // comment this line to disable multi-threaded TX/RX

// global parameters
constexpr bool enable_logs = false;
#ifdef MULTI_THREADED
constexpr bool active_waiting = false;
const size_t n_threads = std::thread::hardware_concurrency();
#endif /* MULTI_THREADED */

// aliases
template<class T> using uptr = std::unique_ptr<T>;

int main(int argc, char** argv)
{
	// setup signal handlers
	spu::tools::Signal_handler::init();

	// get the parameter to configure the tools and modules
	auto params = factory::DVBS2(argc, argv);

	std::cout << "[trace]" << std::endl;
	std::map<std::string,tools::header_list> headers;
	std::vector<factory::Factory*> param_vec;
	param_vec.push_back(&params);
	tools::Header::print_parameters(param_vec, false, std::cout);

	// the list of the allocated modules for the simulation
	std::vector<const spu::module::Module*> modules_each_snr;

	// construct tools
	tools::Constellation_user<float> cstl(params.constellation_file);
	tools::BCH_polynomial_generator<> poly_gen(params.N_bch_unshortened, 12, params.bch_prim_poly);
	tools::Gaussian_noise_generator_fast<R> gen(0);
	std::unique_ptr<tools::Interleaver_core<>> itl_core(factory::DVBS2::build_itl_core<>(params));
	tools::Sigma<> noise_ref;

	// construct modules
	uptr<spu::module::Source<>       > source        (factory::DVBS2::build_source                   <>(params             ));
	uptr<spu::module::Sink<>         > sink          (factory::DVBS2::build_sink                     <>(params             ));
	uptr<Channel<>                   > channel       (factory::DVBS2::build_channel                  <>(params, gen        ));
	uptr<Scrambler<>                 > bb_scrambler  (factory::DVBS2::build_bb_scrambler             <>(params             ));
	uptr<Scrambler<>                 > bb_descrambler(factory::DVBS2::build_bb_scrambler             <>(params             ));
	uptr<Encoder<>                   > BCH_encoder   (factory::DVBS2::build_bch_encoder              <>(params, poly_gen   ));
	uptr<Decoder_HIHO<>              > BCH_decoder   (factory::DVBS2::build_bch_decoder              <>(params, poly_gen   ));
	uptr<tools::Codec_SIHO<>         > LDPC_cdc      (factory::DVBS2::build_ldpc_cdc                 <>(params             ));
	uptr<Interleaver<>               > itl_tx        (factory::DVBS2::build_itl                      <>(params, *itl_core  ));
	uptr<Interleaver<float,uint32_t> > itl_rx        (factory::DVBS2::build_itl<float,uint32_t>        (params, *itl_core  ));
	uptr<Modem<>                     > modem         (factory::DVBS2::build_modem                    <>(params, &cstl      ));
	uptr<Filter_UPRRC_ccr_naive<>    > shaping_flt   (factory::DVBS2::build_uprrc_filter             <>(params             ));
	uptr<Multiplier_sine_ccc_naive<> > freq_shift    (factory::DVBS2::build_freq_shift               <>(params             ));
	uptr<Filter_Farrow_ccr_naive<>   > chn_frac_del  (factory::DVBS2::build_channel_frac_delay       <>(params             ));
	uptr<Variable_delay_cc_naive<>   > chn_int_del   (factory::DVBS2::build_channel_int_delay        <>(params             ));
	uptr<Filter_buffered_delay<float>> chn_frm_del   (factory::DVBS2::build_channel_frame_delay      <>(params             ));
	uptr<Synchronizer_frame<>        > sync_frame    (factory::DVBS2::build_synchronizer_frame       <>(params             ));
	uptr<Feedbacker<>                > feedbr        (factory::DVBS2::build_feedbacker               <>(params             ));
	uptr<Framer<>                    > framer        (factory::DVBS2::build_framer                   <>(params             ));
	uptr<Scrambler<float>            > pl_scrambler  (factory::DVBS2::build_pl_scrambler             <>(params             ));
	uptr<Filter_buffered_delay<>     > delay         (factory::DVBS2::build_txrx_delay               <>(params             ));
	uptr<Monitor_BFER<>              > monitor       (factory::DVBS2::build_monitor                  <>(params             ));
	uptr<Filter_RRC_ccr_naive<>      > matched_flt   (factory::DVBS2::build_matched_filter           <>(params             ));
	uptr<Synchronizer_timing<>       > sync_timing   (factory::DVBS2::build_synchronizer_timing      <>(params             ));
	uptr<Multiplier_AGC_cc_naive<>   > mult_agc      (factory::DVBS2::build_agc_shift                <>(params             ));
	uptr<Estimator<>                 > estimator     (factory::DVBS2::build_estimator                <>(params, &noise_ref ));
	uptr<Multiplier_fading_DVBS2<>   > fad_mlt       (factory::DVBS2::build_fading_mult              <>(params             ));
	uptr<Synchronizer_freq_coarse<>  > sync_coarse_f (factory::DVBS2::build_synchronizer_freq_coarse <>(params             ));
	uptr<Synchronizer_freq_fine<>    > sync_fine_pf  (factory::DVBS2::build_synchronizer_freq_phase  <>(params             ));
	uptr<Synchronizer_freq_fine<>    > sync_fine_lr  (factory::DVBS2::build_synchronizer_lr          <>(params             ));
	uptr<Synchronizer_step_mf_cc<>   > sync_step_mf  (factory::DVBS2::build_synchronizer_step_mf_cc  <>(params,
	                                                                                                    sync_coarse_f.get(),
	                                                                                                    matched_flt  .get(),
	                                                                                                    sync_timing  .get()));
	auto* LDPC_encoder = &LDPC_cdc->get_encoder();
	auto* LDPC_decoder = &LDPC_cdc->get_decoder_siho();

	const size_t probe_buff = 200;
	// create reporters and probes for the statistics file
	spu::tools::Reporter_probe rep_fra_stats("Counter");
	spu::module::Probe_occurrence prb_fra_id("FRAME");
	spu::module::Probe_stream prb_fra_sid("STREAM");
	rep_fra_stats.register_probes({ &prb_fra_id, &prb_fra_sid });
	rep_fra_stats.set_cols_buff_size(probe_buff);
	rep_fra_stats.set_n_frames(params.n_frames);

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

	spu::tools::Reporter_probe rep_noise_rea_stats("Signal Noise Ratio", "Real (SNR)");
	spu::module::Probe_value<float> prb_noise_rsig(1, "SIGMA");
	spu::module::Probe_value<float> prb_noise_res(1, "Es/N0");
	spu::module::Probe_value<float> prb_noise_reb(1, "Eb/N0");
	rep_noise_rea_stats.register_probes({ &prb_noise_rsig, &prb_noise_res, &prb_noise_reb });
	prb_noise_rsig.set_col_prec(4);
	prb_noise_res.set_col_unit("(dB)");
	prb_noise_reb.set_col_unit("(dB)");
	rep_noise_rea_stats.set_cols_fmtflags(std::ios_base::dec | std::ios_base::fixed);
	rep_noise_rea_stats.set_cols_buff_size(probe_buff);
	rep_noise_rea_stats.set_n_frames(params.n_frames);

	spu::tools::Reporter_probe rep_noise_est_stats("Signal Noise Ratio", "Estimated (SNR)");
	spu::module::Probe_value<float> prb_noise_esig(1, "SIGMA");
	spu::module::Probe_value<float> prb_noise_ees(1, "Es/N0");
	spu::module::Probe_value<float> prb_noise_eeb(1, "Eb/N0");
	rep_noise_est_stats.register_probes({ &prb_noise_esig, &prb_noise_ees, &prb_noise_eeb });
	prb_noise_esig.set_col_prec(4);
	prb_noise_ees.set_col_unit("(dB)");
	prb_noise_eeb.set_col_unit("(dB)");
	rep_noise_est_stats.set_cols_fmtflags(std::ios_base::dec | std::ios_base::fixed);
	rep_noise_est_stats.set_cols_buff_size(probe_buff);
	rep_noise_est_stats.set_n_frames(params.n_frames);

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
	spu::module::Probe_latency prb_thr_lat("LAT");
	spu::module::Probe_time prb_thr_time("TIME");
	spu::module::Probe_timestamp prb_thr_tsta("TSTA");
	rep_thr_stats.register_probes({ &prb_thr_thr, &prb_thr_lat, &prb_thr_time, &prb_thr_tsta });
	prb_thr_thr.set_col_unit("(Mbps)");
	rep_thr_stats.set_cols_buff_size(probe_buff);
	rep_thr_stats.set_n_frames(params.n_frames);

	spu::tools::Terminal_dump terminal_stats({ &rep_fra_stats,       &rep_sfm_stats,     &rep_stm_stats,
	                                           &rep_frq_stats,       &rep_decstat_stats, &rep_noise_rea_stats,
	                                           &rep_noise_est_stats, &rep_BFER_stats,    &rep_thr_stats });

	delay        ->set_custom_name("TX/RX Delay");
	LDPC_encoder ->set_custom_name("LDPC Encoder");
	LDPC_decoder ->set_custom_name("LDPC Decoder");
	BCH_encoder  ->set_custom_name("BCH Encoder" );
	BCH_decoder  ->set_custom_name("BCH Decoder" );
	sync_fine_lr ->set_custom_name("L&R F Syn"   );
	sync_fine_pf ->set_custom_name("Fine P/F Syn");
	sync_timing  ->set_custom_name("Timing Syn"  );
	sync_frame   ->set_custom_name("Frame Syn"   );
	matched_flt  ->set_custom_name("Matched Flt" );
	shaping_flt  ->set_custom_name("Shaping Flt" );
	sync_coarse_f->set_custom_name("Coarse_Synch");
	sync_step_mf ->set_custom_name("MF_Synch"    );
	freq_shift   ->set_custom_name("Mult Freq."  );
	mult_agc     ->set_custom_name("Mult AGC"    );
	chn_int_del  ->set_custom_name("Chn int del ");
	chn_frac_del ->set_custom_name("Chn frac del");

	// socket binding of the full transmission sequence
	std::vector<float> sigma(params.n_frames);
	// delay line for BER/FER
	(*delay         )[             flt::sck::filter       ::X_N1   ] = (*source        )[spu::module::src::sck::generate     ::out_data];
	// TX
	(*bb_scrambler  )[             scr::sck::scramble     ::X_N1   ] = (*source        )[spu::module::src::sck::generate     ::out_data];
	(*BCH_encoder   )[             enc::sck::encode       ::U_K    ] = (*bb_scrambler  )[             scr::sck::scramble     ::X_N2    ];
	(*LDPC_encoder  )[             enc::sck::encode       ::U_K    ] = (*BCH_encoder   )[             enc::sck::encode       ::X_N     ];
	(*itl_tx        )[             itl::sck::interleave   ::nat    ] = (*LDPC_encoder  )[             enc::sck::encode       ::X_N     ];
	(*modem         )[             mdm::sck::modulate     ::X_N1   ] = (*itl_tx        )[             itl::sck::interleave   ::itl     ];
	(*framer        )[             frm::sck::generate     ::Y_N1   ] = (*modem         )[             mdm::sck::modulate     ::X_N2    ];
	(*pl_scrambler  )[             scr::sck::scramble     ::X_N1   ] = (*framer        )[             frm::sck::generate     ::Y_N2    ];
	(*shaping_flt   )[             flt::sck::filter       ::X_N1   ] = (*pl_scrambler  )[             scr::sck::scramble     ::X_N2    ];
	// Channel
	(*fad_mlt       )[             mlt::sck::imultiply    ::X_N    ] = (*shaping_flt   )[             flt::sck::filter       ::Y_N2    ];
	(*chn_frm_del   )[             flt::sck::filter       ::X_N1   ] = (*fad_mlt       )[             mlt::sck::imultiply    ::Z_N     ];
	(*chn_int_del   )[             flt::sck::filter       ::X_N1   ] = (*chn_frm_del   )[             flt::sck::filter       ::Y_N2    ];
	(*chn_frac_del  )[             flt::sck::filter       ::X_N1   ] = (*chn_int_del   )[             flt::sck::filter       ::Y_N2    ];
	(*freq_shift    )[             mlt::sck::imultiply    ::X_N    ] = (*chn_frac_del  )[             flt::sck::filter       ::Y_N2    ];
	(*channel       )[             chn::sck::add_noise    ::CP     ] =                                                         sigma    ;
	(*channel       )[             chn::sck::add_noise    ::X_N    ] = (*freq_shift    )[             mlt::sck::imultiply    ::Z_N     ];
	// RX
	(*sync_coarse_f )[             sfc::sck::synchronize  ::X_N1   ] = (*channel       )[             chn::sck::add_noise    ::Y_N     ];
	(*matched_flt   )[             flt::sck::filter       ::X_N1   ] = (*sync_coarse_f )[             sfc::sck::synchronize  ::Y_N2    ];
	(*sync_timing   )[             stm::sck::synchronize  ::X_N1   ] = (*matched_flt   )[             flt::sck::filter       ::Y_N2    ];
	(*sync_timing   )[             stm::sck::extract      ::B_N1   ] = (*sync_timing   )[             stm::sck::synchronize  ::B_N1    ];
	(*sync_timing   )[             stm::sck::extract      ::Y_N1   ] = (*sync_timing   )[             stm::sck::synchronize  ::Y_N1    ];
	(*mult_agc      )[             mlt::sck::imultiply    ::X_N    ] = (*sync_timing   )[             stm::sck::extract      ::Y_N2    ];
	(*sync_frame    )[             sfm::sck::synchronize  ::X_N1   ] = (*mult_agc      )[             mlt::sck::imultiply    ::Z_N     ];
	(*pl_scrambler  )[             scr::sck::descramble   ::Y_N1   ] = (*sync_frame    )[             sfm::sck::synchronize  ::Y_N2    ];
	(*sync_fine_lr  )[             sff::sck::synchronize  ::X_N1   ] = (*pl_scrambler  )[             scr::sck::descramble   ::Y_N2    ];
	(*sync_fine_pf  )[             sff::sck::synchronize  ::X_N1   ] = (*sync_fine_lr  )[             sff::sck::synchronize  ::Y_N2    ];
	(*framer        )[             frm::sck::remove_plh   ::Y_N1   ] = (*sync_fine_pf  )[             sff::sck::synchronize  ::Y_N2    ];
	(*estimator     )[             est::sck::estimate     ::X_N    ] = (*framer        )[             frm::sck::remove_plh   ::Y_N2    ];
	(*modem         )[             mdm::sck::demodulate   ::CP     ] = (*estimator     )[             est::sck::estimate     ::SIG     ];
	(*modem         )[             mdm::sck::demodulate   ::Y_N1   ] = (*framer        )[             frm::sck::remove_plh   ::Y_N2    ];
	(*itl_rx        )[             itl::sck::deinterleave ::itl    ] = (*modem         )[             mdm::sck::demodulate   ::Y_N2    ];
	(*LDPC_decoder  )[             dec::sck::decode_siho  ::Y_N    ] = (*itl_rx        )[             itl::sck::deinterleave ::nat     ];
	(*BCH_decoder   )[             dec::sck::decode_hiho  ::Y_N    ] = (*LDPC_decoder  )[             dec::sck::decode_siho  ::V_K     ];
	(*bb_descrambler)[             scr::sck::descramble   ::Y_N1   ] = (*BCH_decoder   )[             dec::sck::decode_hiho  ::V_K     ];
	(*monitor       )[             mnt::sck::check_errors2::U      ] = (*delay         )[             flt::sck::filter       ::Y_N2    ];
	(*monitor       )[             mnt::sck::check_errors2::V      ] = (*bb_descrambler)[             scr::sck::descramble   ::Y_N2    ];
	(*sink          )[spu::module::snk::sck::send         ::in_data] = (*bb_descrambler)[             scr::sck::descramble   ::Y_N2    ];
	// bind the RX probes
	prb_frq_coa      [spu::module::prb::sck::probe        ::in     ] = (*sync_coarse_f )[             sfc::sck::synchronize  ::FRQ     ];
	prb_stm_del      [spu::module::prb::sck::probe        ::in     ] = (*sync_timing   )[             stm::sck::synchronize  ::MU      ];
	prb_stm_uff      [spu::module::prb::sck::probe        ::in     ] = (*sync_timing   )[             stm::sck::extract      ::UFW     ];
	prb_sfm_del      [spu::module::prb::sck::probe        ::in     ] = (*sync_frame    )[             sfm::sck::synchronize  ::DEL     ];
	prb_sfm_tri      [spu::module::prb::sck::probe        ::in     ] = (*sync_frame    )[             sfm::sck::synchronize  ::TRI     ];
	prb_sfm_flg      [spu::module::prb::sck::probe        ::in     ] = (*sync_frame    )[             sfm::sck::synchronize  ::FLG     ];
	prb_frq_lr       [spu::module::prb::sck::probe        ::in     ] = (*sync_fine_lr  )[             sff::sck::synchronize  ::FRQ     ];
	prb_frq_fin      [spu::module::prb::sck::probe        ::in     ] = (*sync_fine_pf  )[             sff::sck::synchronize  ::FRQ     ];
	prb_noise_ees    [spu::module::prb::sck::probe        ::in     ] = (*estimator     )[             est::sck::estimate     ::Es_N0   ];
	prb_noise_eeb    [spu::module::prb::sck::probe        ::in     ] = (*estimator     )[             est::sck::estimate     ::Eb_N0   ];
	prb_noise_esig   [spu::module::prb::sck::probe        ::in     ] = (*estimator     )[             est::sck::estimate     ::SIG     ];
	prb_decstat_ldpc [spu::module::prb::sck::probe        ::in     ] = (*LDPC_decoder  )[             dec::sck::decode_siho  ::CWD     ];
	prb_decstat_bch  [spu::module::prb::sck::probe        ::in     ] = (*BCH_decoder   )[             dec::sck::decode_hiho  ::CWD     ];
	prb_thr_thr      [spu::module::prb::tsk::probe                 ] = (*bb_descrambler)[             scr::sck::descramble   ::Y_N2    ];
	prb_thr_lat      [spu::module::prb::tsk::probe                 ] = (*sink          )[spu::module::snk::sck::send         ::status  ];
	prb_thr_time     [spu::module::prb::tsk::probe                 ] = (*sink          )[spu::module::snk::sck::send         ::status  ];
	prb_thr_tsta     [spu::module::prb::tsk::probe                 ] = (*sink          )[spu::module::snk::sck::send         ::status  ];
	prb_bfer_be      [spu::module::prb::sck::probe        ::in     ] = (*monitor       )[             mnt::sck::check_errors2::BE      ];
	prb_bfer_fe      [spu::module::prb::sck::probe        ::in     ] = (*monitor       )[             mnt::sck::check_errors2::FE      ];
	prb_bfer_ber     [spu::module::prb::sck::probe        ::in     ] = (*monitor       )[             mnt::sck::check_errors2::BER     ];
	prb_bfer_fer     [spu::module::prb::sck::probe        ::in     ] = (*monitor       )[             mnt::sck::check_errors2::FER     ];
	prb_fra_id       [spu::module::prb::tsk::probe                 ] = (*sink          )[spu::module::snk::sck::send         ::status  ];
	prb_fra_sid      [spu::module::prb::tsk::probe                 ] = (*sink          )[spu::module::snk::sck::send         ::status  ];

	// first stages of the whole transmission sequence
	const std::vector<spu::runtime::Task*> firsts_t = { &(*source     )[spu::module::src::tsk::generate],
	                                                    &prb_noise_res [spu::module::prb::tsk::probe   ],
	                                                    &prb_noise_reb [spu::module::prb::tsk::probe   ],
	                                                    &prb_noise_rsig[spu::module::prb::tsk::probe   ] };

#ifdef MULTI_THREADED
	auto start_clone = std::chrono::system_clock::now();
	std::cout << "Cloning the modules of the parallel sequence... ";
	std::cout.flush();

	// pipeline definition with separation stages
	const std::vector<std::tuple<std::vector<spu::runtime::Task*>,
	                             std::vector<spu::runtime::Task*>,
	                             std::vector<spu::runtime::Task*>>> sep_stages =
	{ // pipeline stage 0 -> TX
	  std::make_tuple<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*source)[spu::module::src::tsk::generate], &(*delay)[flt::tsk::filter],
	      &prb_noise_res[spu::module::prb::tsk::probe], &prb_noise_reb[spu::module::prb::tsk::probe],
	      &prb_noise_rsig[spu::module::prb::tsk::probe] },
	    { &(*source)[spu::module::src::tsk::generate] },
	    { /* no exclusions in this stage */ } ),
	  // pipeline stage 1 -> TX
	  std::make_tuple<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*bb_scrambler)[scr::tsk::scramble] },
	    { &(*pl_scrambler)[scr::tsk::scramble] },
	    { /* no exclusions in this stage */ } ),
	  // pipeline stage 2 -> TX
	  std::make_tuple<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*shaping_flt)[flt::tsk::filter] },
	    { &(*shaping_flt)[flt::tsk::filter] },
	    { /* no exclusions in this stage */ } ),
	  // pipeline stage 3 -> channel
	  std::make_tuple<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*fad_mlt)[mlt::tsk::imultiply] },
	    { &(*channel)[chn::tsk::add_noise] },
	    { /* no exclusions in this stage */ } ),
	  // pipeline stage 4 -> RX
	  std::make_tuple<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*sync_coarse_f)[sfc::tsk::synchronize] },
	    { &(*matched_flt)[flt::tsk::filter], &prb_frq_coa[spu::module::prb::tsk::probe] },
	    { /* no exclusions in this stage */ } ),
	  // pipeline stage 5 -> RX
	  std::make_tuple<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*sync_timing)[stm::tsk::synchronize], &prb_stm_del[spu::module::prb::tsk::probe] },
	    { &(*sync_timing)[stm::tsk::synchronize] },
	    { /* no exclusions in this stage */ } ),
	  // pipeline stage 6 -> RX
	  std::make_tuple<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*sync_timing)[stm::tsk::extract], &prb_sfm_del[spu::module::prb::tsk::probe],
	      &prb_sfm_tri[spu::module::prb::tsk::probe], &prb_sfm_flg[spu::module::prb::tsk::probe] },
	    { &(*sync_frame)[sfm::tsk::synchronize] },
	    { /* no exclusions in this stage */ } ),
	  // pipeline stage 7 -> RX
	  std::make_tuple<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*pl_scrambler)[scr::tsk::descramble], &prb_frq_fin[spu::module::prb::tsk::probe] },
	    { &(*sync_fine_pf)[sff::tsk::synchronize], &prb_frq_lr[spu::module::prb::tsk::probe] },
	    { /* no exclusions in this stage */ } ),
	  // pipeline stage 8 -> RX
	  std::make_tuple<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*framer)[frm::tsk::remove_plh], &prb_noise_esig[spu::module::prb::tsk::probe],
	      &prb_noise_ees[spu::module::prb::tsk::probe], &prb_noise_eeb[spu::module::prb::tsk::probe] },
	    { &(*estimator)[est::tsk::estimate] },
	    { &(*modem)[mdm::tsk::demodulate] } ),
	  // pipeline stage 9 -> RX
	  std::make_tuple<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*modem)[mdm::tsk::demodulate] },
	    { &(*bb_descrambler)[scr::tsk::descramble] },
	    { &prb_decstat_ldpc[spu::module::prb::tsk::probe], &prb_decstat_bch[spu::module::prb::tsk::probe],
	      &prb_thr_thr[spu::module::prb::tsk::probe] } ),
	  // pipeline stage 10 -> RX
	  std::make_tuple<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*monitor)[mnt::tsk::check_errors2], &(*sink)[spu::module::snk::tsk::send],
	      &prb_decstat_ldpc[spu::module::prb::tsk::probe], &prb_decstat_bch[spu::module::prb::tsk::probe],
	      &prb_thr_thr[spu::module::prb::tsk::probe] },
	    { /* end of the sequence */ },
	    { /* no exclusions in this stage */ } ),
	};
	// number of threads per stages
	const std::vector<size_t> n_threads_per_stages = { 1, n_threads, 1, 1, 1, 1, 1, 1, 1, n_threads, 1 };
	// synchronization buffer size between stages
	const std::vector<size_t> buffer_sizes(sep_stages.size() -1, 1);
	// type of waiting between stages (true = active, false = passive)
	const std::vector<bool> active_waitings(sep_stages.size() -1, active_waiting);

	spu::runtime::Pipeline pipeline_transmission(firsts_t, sep_stages, n_threads_per_stages, buffer_sizes,
                                                 active_waitings);

	if (enable_logs)
	{
		std::ofstream f("tx_rx_pipeline_transmission.dot");
		pipeline_transmission.export_dot(f);
	}

	auto tasks_per_types = pipeline_transmission.get_tasks_per_types();
	auto end_clone = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds_clone = end_clone - start_clone;
	std::cout << "Done (" << elapsed_seconds_clone.count() << "s)." << std::endl;
#else
	runtime::Sequence sequence_transmission(firsts_t);
	if (enable_logs)
	{
		std::ofstream f("tx_rx_sequence_transmission.dot");
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

	// allocate reporters to display results in the terminal_stats
	tools::Reporter_noise<>      rep_noise( noise_ref, true);
	tools::Reporter_BFER<>       rep_BFER (*monitor        );
	tools::Reporter_throughput<> rep_thr  (*monitor        );

	// allocate a terminal that will display the collected data from the reporters
	spu::tools::Terminal_std terminal({ &rep_noise, &rep_BFER, &rep_thr });

	// display the legend in the terminal
	terminal.legend();

	// a loop over the various SNRs
	std::mt19937 prng;
	for (auto ebn0 = params.ebn0_min; ebn0 < params.ebn0_max; ebn0 += params.ebn0_step)
	{
		// compute the code rate
		const float R = (float)params.K_bch / (float)params.N_ldpc;

		// compute the current sigma for the channel noise
		const auto esn0  = tools::ebn0_to_esn0 (ebn0, R, params.bps);
		std::fill(sigma.begin(), sigma.end(), tools::esn0_to_sigma(esn0));

		std::vector<float> esvec (params.n_frames, esn0    );
		std::vector<float> ebvec (params.n_frames, ebn0    );
		std::vector<float> sigvec(params.n_frames, sigma[0]);

		prb_noise_res [spu::module::prb::sck::probe::in].bind(esvec );
		prb_noise_reb [spu::module::prb::sck::probe::in].bind(ebvec );
		prb_noise_rsig[spu::module::prb::sck::probe::in].bind(sigvec);

		std::stringstream sigma_ss;
		sigma_ss << std::fixed << std::setprecision(3) << sigma[0];

		noise_ref.set_values(sigma[0], ebn0, esn0);

		// reset synchro modules
		shaping_flt  ->reset(); chn_frac_del->reset(); chn_int_del->reset(); freq_shift ->reset();
		sync_coarse_f->reset(); matched_flt ->reset(); sync_timing->reset(); sync_frame ->reset();
		sync_fine_lr ->reset(); sync_fine_pf->reset(); delay      ->reset(); chn_frm_del->reset();
		prb_fra_id    .reset(); prb_fra_sid  .reset();

		int delay_tx_rx = params.overall_delay;
		delay->set_delay(delay_tx_rx);

		std::ofstream stats_file("stats_sigma_" + sigma_ss.str() + ".txt");

		if (params.ter_freq != std::chrono::nanoseconds(0))
			terminal.start_temp_report(params.ter_freq);

		source ->set_seed(prng());
		channel->set_seed(prng());
		if (!params.perfect_sync)
		{
#ifdef MULTI_THREADED
			pipeline_transmission.unbind_adaptors();
#endif /* MULTI_THREADED */
			// ========================================================================================================
			// WAITING PHASE ==========================================================================================
			// ========================================================================================================
			// display the legend in the terminal
			std::ofstream waiting_stats("waiting_stats_sigma_" + sigma_ss.str() + ".txt");
			waiting_stats << "#################" << std::endl;
			waiting_stats << "# WAITING PHASE #" << std::endl;
			waiting_stats << "#################" << std::endl;
			terminal_stats.legend(waiting_stats);

			// partial unbinding
			(*sync_coarse_f)[             sfc::sck::synchronize::X_N1].unbind((*channel      )[chn::sck::add_noise  ::Y_N ]);
			(*sync_timing  )[             stm::sck::extract    ::B_N1].unbind((*sync_timing  )[stm::sck::synchronize::B_N1]);
			(*sync_timing  )[             stm::sck::extract    ::Y_N1].unbind((*sync_timing  )[stm::sck::synchronize::Y_N1]);
			prb_frq_coa     [spu::module::prb::sck::probe      ::in  ].unbind((*sync_coarse_f)[sfc::sck::synchronize::FRQ ]);
			prb_stm_del     [spu::module::prb::sck::probe      ::in  ].unbind((*sync_timing  )[stm::sck::synchronize::MU  ]);

			// partial binding
			(*sync_step_mf)[             smf::sck::synchronize::X_N1] = (*channel     )[chn::sck::add_noise  ::Y_N ];
			(*feedbr      )[             fbr::sck::memorize   ::X_N ] = (*sync_frame  )[sfm::sck::synchronize::DEL ];
			(*sync_step_mf)[             smf::sck::synchronize::DEL ] = (*feedbr      )[fbr::sck::produce    ::Y_N ];
			(*sync_timing )[             stm::sck::extract    ::B_N1] = (*sync_step_mf)[smf::sck::synchronize::B_N1];
			(*sync_timing )[             stm::sck::extract    ::Y_N1] = (*sync_step_mf)[smf::sck::synchronize::Y_N1];
			prb_frq_coa    [spu::module::prb::sck::probe      ::in  ] = (*sync_step_mf)[smf::sck::synchronize::FRQ ];
			prb_stm_del    [spu::module::prb::sck::probe      ::in  ] = (*sync_step_mf)[smf::sck::synchronize::MU  ];

			std::vector<spu::runtime::Task*> firsts_wl12 = { &(*source    )[spu::module::src::tsk::generate],
			                                                 &prb_sfm_del  [spu::module::prb::tsk::probe   ],
			                                                 &prb_sfm_tri  [spu::module::prb::tsk::probe   ],
			                                                 &prb_sfm_flg  [spu::module::prb::tsk::probe   ],
			                                                 &prb_thr_lat  [spu::module::prb::tsk::probe   ],
			                                                 &prb_thr_time [spu::module::prb::tsk::probe   ],
			                                                 &prb_thr_tsta [spu::module::prb::tsk::probe   ],
			                                                 &prb_fra_id   [spu::module::prb::tsk::probe   ],
			                                                 &prb_fra_sid  [spu::module::prb::tsk::probe   ],
			                                                 &(*feedbr    )[             fbr::tsk::produce ] };

			std::vector<spu::runtime::Task*> lasts_wl12 = { &(*feedbr)[fbr::tsk::memorize] };
			std::vector<spu::runtime::Task*> exclude_wl12 = { &(*pl_scrambler)[scr::tsk::descramble] };

			prb_thr_thr .reset();
			prb_thr_lat .reset();
			prb_thr_time.reset();
			spu::runtime::Sequence sequence_waiting_and_learning_1_2(firsts_wl12, lasts_wl12, exclude_wl12);

			if (enable_logs && ebn0 == params.ebn0_min)
			{
				std::ofstream f("tx_rx_sequence_waiting_and_learning_1_2.dot");
				sequence_waiting_and_learning_1_2.export_dot(f);
			}

			sync_coarse_f->set_PLL_coeffs(1, 1/std::sqrt(2.0), 1e-4);
			sequence_waiting_and_learning_1_2.exec([&](const std::vector<const int*>& statuses)
			{
				const auto m = prb_fra_id.get_occurrences();
				if (statuses.back() != nullptr)
				{
					terminal_stats.temp_report(waiting_stats);
				}
				else
				{
					delay_tx_rx += params.n_frames;
					if (enable_logs)
						std::clog << rang::tag::warning << "Sequence aborted! (waiting phase, m = " << m << ")"
						          << std::endl;
				}

				return sync_frame->get_packet_flag();
			});
			prb_fra_id.reset();
			prb_fra_sid.reset();

			// ========================================================================================================
			// LEARNING PHASE 1 & 2 ===================================================================================
			// ========================================================================================================
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
				else
				{
					delay_tx_rx += params.n_frames;
					if (enable_logs)
						std::clog << rang::tag::warning << "Sequence aborted! (learning phase 1&2, m = " << m << ")"
						          << std::endl;
				}

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

			// ========================================================================================================
			// LEARNING PHASE 3 =======================================================================================
			// ========================================================================================================
			stats_file << "####################" << std::endl;
			stats_file << "# LEARNING PHASE 3 #" << std::endl;
			stats_file << "####################" << std::endl;
			terminal_stats.legend(stats_file);

			// partial unbinding
			(*sync_step_mf)[             smf::sck::synchronize::X_N1].unbind((*channel     )[chn::sck::add_noise  ::Y_N ]);
			(*feedbr      )[             fbr::sck::memorize   ::X_N ].unbind((*sync_frame  )[sfm::sck::synchronize::DEL ]);
			(*sync_timing )[             stm::sck::extract    ::B_N1].unbind((*sync_step_mf)[smf::sck::synchronize::B_N1]);
			(*sync_timing )[             stm::sck::extract    ::Y_N1].unbind((*sync_step_mf)[smf::sck::synchronize::Y_N1]);
			prb_frq_coa    [spu::module::prb::sck::probe      ::in  ].unbind((*sync_step_mf)[smf::sck::synchronize::FRQ ]);
			prb_stm_del    [spu::module::prb::sck::probe      ::in  ].unbind((*sync_step_mf)[smf::sck::synchronize::MU  ]);

			// partial binding
			(*sync_coarse_f)[             sfc::sck::synchronize::X_N1] = (*channel      )[chn::sck::add_noise  ::Y_N ];
			(*sync_timing  )[             stm::sck::extract    ::B_N1] = (*sync_timing  )[stm::sck::synchronize::B_N1];
			(*sync_timing  )[             stm::sck::extract    ::Y_N1] = (*sync_timing  )[stm::sck::synchronize::Y_N1];
			prb_frq_coa     [spu::module::prb::sck::probe      ::in  ] = (*sync_coarse_f)[sfc::sck::synchronize::FRQ ];
			prb_stm_del     [spu::module::prb::sck::probe      ::in  ] = (*sync_timing  )[stm::sck::synchronize::MU  ];

			std::vector<spu::runtime::Task*> firsts_l3 = { &(*source   )[spu::module::src::tsk::generate],
			                                               &prb_thr_lat [spu::module::prb::tsk::probe   ],
			                                               &prb_thr_time[spu::module::prb::tsk::probe   ],
			                                               &prb_thr_tsta[spu::module::prb::tsk::probe   ],
			                                               &prb_fra_id  [spu::module::prb::tsk::probe   ],
			                                               &prb_fra_sid [spu::module::prb::tsk::probe   ],
			                                               &prb_frq_fin [spu::module::prb::tsk::probe   ] };

			std::vector<spu::runtime::Task*> lasts_l3 = { &(*sync_fine_pf)[sff::tsk::synchronize] };

			spu::runtime::Sequence sequence_learning_3(firsts_l3, lasts_l3);

			if (enable_logs && ebn0 == params.ebn0_min)
			{
				std::ofstream f("tx_rx_sequence_learning_3.dot");
				sequence_learning_3.export_dot(f);
			}

			limit = prb_fra_id.get_occurrences() + 200;
			prb_thr_thr.reset();
			prb_thr_lat.reset();
			sequence_learning_3.exec([&](const std::vector<const int*>& statuses)
			{
				const auto m = prb_fra_id.get_occurrences();
				if (statuses.back() != nullptr)
					terminal_stats.temp_report(stats_file);
				else
				{
					delay_tx_rx += params.n_frames;
					if (enable_logs)
						std::clog << rang::tag::warning << "Sequence aborted! (learning phase 3, m = " << m << ")"
						          << std::endl;
				}
				return m >= limit;
			});
		}
		monitor->reset();

		// ============================================================================================================
		// TRANSMISSION PHASE =========================================================================================
		// ============================================================================================================
		stats_file << "######################" << std::endl;
		stats_file << "# TRANSMISSION PHASE #" << std::endl;
		stats_file << "######################" << std::endl;
		terminal_stats.legend(stats_file);

		// reset the statistics of the tasks before the transmission phase
		for (auto &tt : tasks_per_types)
			for (auto &t : tt)
				t->reset();

		delay->set_delay(delay_tx_rx);
		sync_timing->set_act(true);

		auto stop_condition = [&monitor](const std::vector<const int*>&) {
			return monitor->is_done();
		};

		int d = 0;
		prb_thr_thr.reset();
		prb_thr_lat.reset();
#ifdef MULTI_THREADED
		pipeline_transmission.bind_adaptors();
		pipeline_transmission.exec({
			stop_condition, // stop condition stage  0
			stop_condition, // stop condition stage  1
			stop_condition, // stop condition stage  2
			stop_condition, // stop condition stage  3
			stop_condition, // stop condition stage  4
			stop_condition, // stop condition stage  5
			                // stop condition stage  6
			[&stop_condition, &prb_fra_id] (const std::vector<const int*>& statuses)
			{
				if (statuses.back() == nullptr && enable_logs)
					std::clog << std::endl << rang::tag::warning << "Sequence aborted! (transmission phase, stage = 5"
					          << ", m = " << prb_fra_id.get_occurrences() << ")" << std::endl;
				return stop_condition(statuses);
			},
			stop_condition, // stop condition stage  7
			stop_condition, // stop condition stage  8
			stop_condition, // stop condition stage  9
			                // stop condition stage 10
			[&monitor, &stop_condition, &terminal_stats, &stats_file, &delay_tx_rx, &d, &params]
			(const std::vector<const int*>& statuses)
			{
				d += params.n_frames;
				terminal_stats.temp_report(stats_file);
				// first frame is delayed
				if (d < delay_tx_rx + params.n_frames)
					monitor->reset();
				return stop_condition(statuses);
			}
		});
#else
		sequence_transmission.exec([&](const std::vector<int>& statuses)
		{
			if (statuses.back() != status_t::SKIPPED)
			{
				d += params.n_frames;
				terminal_stats.temp_report(stats_file);
			}
			else if (enable_logs)
			{
				const auto m = prb_fra_id.get_occurrences();
				std::clog << rang::tag::warning << "Sequence aborted! (Transmisison phase, m = " << m << ")"
				                                << std::endl;
			}

			if (d < delay_tx_rx + params.n_frames) // first frame is delayed
				monitor->reset();

			return stop_condition(statuses);
		});
#endif /* MULTI_THREADED */

		// display the performance (BER and FER) in the terminal
		terminal.final_report();

		auto reporters = terminal_stats.get_reporters();
		for (auto reporter : reporters)
			reporter->reset();

		// reset the monitors and the terminal for the next SNR
		monitor->reset();

		if (params.stats)
		{
			const auto ordered = true;
#ifdef MULTI_THREADED
			auto stages = pipeline_transmission.get_stages();
			for (size_t ss = 0; ss < stages.size(); ss++)
			{
				std::cout << "#" << std::endl << "# Sequence stage " << ss << " (" << stages[ss]->get_n_threads()
				          << " thread(s)): " << std::endl;
				spu::tools::Stats::show(stages[ss]->get_tasks_per_types(), ordered);
			}
#else
			std::cout << "#" << std::endl << "# Sequence sequential (" << sequence_transmission.get_n_threads()
			          << " thread(s)): " << std::endl;
			spu::tools::Stats::show(sequence_transmission.get_tasks_per_types(), ordered);
#endif /* MULTI_THREADED */
			for (auto &tt : tasks_per_types)
				for (auto &t : tt)
					t->reset();

			if (ebn0 + params.ebn0_step < params.ebn0_max)
			{
				std::cout << "#" << std::endl;
				terminal.legend();
			}
		}
	}

	std::cout << "#" << std::endl;
	std::cout << "# End of the simulation" << std::endl;

	return EXIT_SUCCESS;
}

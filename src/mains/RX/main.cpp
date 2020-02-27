#include <chrono>
#include <fstream>
#include <aff3ct.hpp>

#include "Module/Probe/Probe.hpp"
#include "Module/Probe/Value/Probe_value.hpp"
#include "Module/Probe/Throughput/Probe_throughput.hpp"
#include "Module/Probe/Latency/Probe_latency.hpp"
#include "Module/Probe/Time/Probe_time.hpp"
#include "Module/Probe/Occurrence/Probe_occurrence.hpp"
#include "Tools/Reporter/Reporter_probe.hpp"
#include "Tools/Reporter/Reporter_probe_decstat.hpp"
#include "Factory/DVBS2O/DVBS2O.hpp"
#ifdef DVBS2O_LINK_UHD
#include "Module/Radio/Radio_USRP/Radio_USRP.hpp"
#endif

using namespace aff3ct;
using namespace aff3ct::module;

#define MULTI_THREADED // comment this line to disable multi-threaded RX

// global parameters
constexpr bool enable_logs = true;
#ifdef MULTI_THREADED
constexpr bool thread_pinnig = true;
constexpr bool active_waiting = false;
#endif

// aliases
template<class T> using uptr = std::unique_ptr<T>;

int main(int argc, char** argv)
{
#ifdef MULTI_THREADED
	if (thread_pinnig)
	{
		tools::Thread_pinning::init();
		// tools::Thread_pinning::set_logs(enable_logs);
		tools::Thread_pinning::pin(0);
	}
#endif

	// get the parameter to configure the tools and modules
	auto params = factory::DVBS2O(argc, argv);

	std::map<std::string,tools::header_list> headers;
	std::vector<factory::Factory*> param_vec;
	param_vec.push_back(&params);
	tools::Header::print_parameters(param_vec, true, std::cout);

	// construct tools
	uptr<tools::Constellation<float>> cstl(new tools::Constellation_user<float>(params.constellation_file));
	uptr<tools::Interleaver_core<>> itl_core(factory::DVBS2O::build_itl_core<>(params));
	tools::BCH_polynomial_generator<> poly_gen(params.N_bch_unshortened, 12, params.bch_prim_poly);

	// construct modules
	uptr<Source<>                   > source       (factory::DVBS2O::build_source                  <>(params            ));
	uptr<Sink<>                     > sink         (factory::DVBS2O::build_sink                    <>(params            ));
	uptr<Radio<>                    > radio        (factory::DVBS2O::build_radio                   <>(params            ));
	uptr<Scrambler<>                > bb_scrambler (factory::DVBS2O::build_bb_scrambler            <>(params            ));
	uptr<Decoder_HIHO<>             > BCH_decoder  (factory::DVBS2O::build_bch_decoder             <>(params, poly_gen  ));
	uptr<tools::Codec_SIHO<>        > LDPC_cdc     (factory::DVBS2O::build_ldpc_cdc                <>(params            ));
	uptr<Interleaver<float,uint32_t>> itl_rx       (factory::DVBS2O::build_itl<float,uint32_t>       (params, *itl_core ));
	uptr<Modem<>                    > modem        (factory::DVBS2O::build_modem                   <>(params, cstl.get()));
	uptr<Multiplier_sine_ccc_naive<>> freq_shift   (factory::DVBS2O::build_freq_shift              <>(params            ));
	uptr<Synchronizer_frame<>       > sync_frame   (factory::DVBS2O::build_synchronizer_frame      <>(params            ));
	uptr<Synchronizer_freq_fine<>   > sync_lr      (factory::DVBS2O::build_synchronizer_lr         <>(params            ));
	uptr<Synchronizer_freq_fine<>   > sync_fine_pf (factory::DVBS2O::build_synchronizer_freq_phase <>(params            ));
	uptr<Framer<>                   > framer       (factory::DVBS2O::build_framer                  <>(params            ));
	uptr<Scrambler<float>           > pl_scrambler (factory::DVBS2O::build_pl_scrambler            <>(params            ));
	uptr<Monitor_BFER<>             > monitor      (factory::DVBS2O::build_monitor                 <>(params            ));
	uptr<Filter_RRC_ccr_naive<>     > matched_flt  (factory::DVBS2O::build_matched_filter          <>(params            ));
	uptr<Synchronizer_timing <>     > sync_timing  (factory::DVBS2O::build_synchronizer_timing     <>(params            ));
	uptr<Multiplier_AGC_cc_naive<>  > front_agc    (factory::DVBS2O::build_channel_agc             <>(params            ));
	uptr<Multiplier_AGC_cc_naive<>  > mult_agc     (factory::DVBS2O::build_agc_shift               <>(params            ));
	uptr<Estimator<>                > estimator    (factory::DVBS2O::build_estimator               <>(params            ));
	uptr<Synchronizer_freq_coarse<> > sync_coarse_f(factory::DVBS2O::build_synchronizer_freq_coarse<>(params            ));
	uptr<Synchronizer_step_mf_cc<>  > sync_step_mf (factory::DVBS2O::build_synchronizer_step_mf_cc <>(params,
	                                                                                                  sync_coarse_f.get(),
	                                                                                                  matched_flt  .get(),
	                                                                                                  sync_timing  .get()));
	auto* LDPC_decoder = &LDPC_cdc->get_decoder_siho();

	// create reporters and probes for the statistics file
	tools::Reporter_probe rep_fra_stats("Frame Counter", params.n_frames);
	uptr<Probe_occurrence<int32_t>> prb_fra_id(rep_fra_stats.create_probe_occurrence<int32_t>("ID"));

	tools::Reporter_probe rep_rad_stats("Radio", params.n_frames);
	std::unique_ptr<module::Probe<int32_t>> prb_rad_ovf(rep_rad_stats.create_probe_value<int32_t>("OVF", "FLAG"));
	std::unique_ptr<module::Probe<int32_t>> prb_rad_seq(rep_rad_stats.create_probe_value<int32_t>("SEQ", "ERR"));

	tools::Reporter_probe rep_sfm_stats("Frame Synchronization", params.n_frames);
	uptr<Probe<int32_t>> prb_sfm_del(rep_sfm_stats.create_probe_value<int32_t>("DEL"));
	uptr<Probe<int32_t>> prb_sfm_flg(rep_sfm_stats.create_probe_value<int32_t>("FLG"));
	uptr<Probe<float  >> prb_sfm_tri(rep_sfm_stats.create_probe_value<float  >("TRI", "", 1, std::ios_base::dec | std::ios_base::fixed));

	tools::Reporter_probe rep_stm_stats("Timing Synchronization", "Gardner Algorithm", params.n_frames);
	uptr<Probe<int32_t>> prb_stm_uff(rep_stm_stats.create_probe_value<int32_t>("UFW", "FLAG"));
	uptr<Probe<float  >> prb_stm_del(rep_stm_stats.create_probe_value<float  >("DEL", "FRAC"));

	tools::Reporter_probe rep_frq_stats("Frequency Synchronization", params.n_frames);
	uptr<Probe<float>> prb_frq_coa(rep_frq_stats.create_probe_value<float>("COA", "CFO"));
	uptr<Probe<float>> prb_frq_lr (rep_frq_stats.create_probe_value<float>("L&R", "CFO"));
	uptr<Probe<float>> prb_frq_fin(rep_frq_stats.create_probe_value<float>("FIN", "CFO"));

	tools::Reporter_probe_decstat rep_decstat_stats("Decoders Decoding Status", "('0' = success, '1' = fail)", params.n_frames);
	uptr<Probe<int32_t>> prb_decstat_ldpc(rep_decstat_stats.create_probe_value<int32_t>("LDPC"));
	uptr<Probe<int32_t>> prb_decstat_bch (rep_decstat_stats.create_probe_value<int32_t>("BCH"));

	tools::Reporter_probe rep_noise_stats("Signal Noise Ratio", "(SNR)", params.n_frames);
	uptr<Probe<float>> prb_noise_es(rep_noise_stats.create_probe_value<float>("Es/N0", "(dB)", 1, std::ios_base::dec | std::ios_base::fixed));
	uptr<Probe<float>> prb_noise_eb(rep_noise_stats.create_probe_value<float>("Eb/N0", "(dB)", 1, std::ios_base::dec | std::ios_base::fixed));

	tools::Reporter_probe rep_BFER_stats("Bit Error Rate (BER)", "and Frame Error Rate (FER)", params.n_frames);
	uptr<Probe<int32_t>> prb_bfer_be (rep_BFER_stats.create_probe_value<int32_t>("BE"));
	uptr<Probe<int32_t>> prb_bfer_fe (rep_BFER_stats.create_probe_value<int32_t>("FE"));
	uptr<Probe<float  >> prb_bfer_ber(rep_BFER_stats.create_probe_value<float  >("BER"));
	uptr<Probe<float  >> prb_bfer_fer(rep_BFER_stats.create_probe_value<float  >("FER"));

	tools::Reporter_probe rep_thr_stats("Throughput", "and elapsed time", params.n_frames);
	uptr<Probe<int32_t>> prb_thr_thr (rep_thr_stats.create_probe_throughput<int32_t>("THR", params.K_bch));
	uptr<Probe<double> > prb_thr_the (rep_thr_stats.create_probe_value     <double >("TTHR", "Theory", 1, std::ios_base::dec | std::ios_base::fixed));
	uptr<Probe<int32_t>> prb_thr_lat (rep_thr_stats.create_probe_latency   <int32_t>("LAT"));
	uptr<Probe<int32_t>> prb_thr_time(rep_thr_stats.create_probe_time      <int32_t>("TIME"));

	std::vector<double> theoretical_thr(params.n_frames, params.p_rad.rx_rate/1e6 * (double)params.K_bch / ((double)params.pl_frame_size * (double)params.osf));
	(*prb_thr_the)[prb::sck::probe::in].bind(theoretical_thr.data());

	tools::Terminal_dump terminal_stats({ &rep_fra_stats, &rep_rad_stats,     &rep_sfm_stats,   &rep_stm_stats,
	                                      &rep_frq_stats, &rep_decstat_stats, &rep_noise_stats, &rep_BFER_stats,
	                                      &rep_thr_stats });

#ifdef MULTI_THREADED
	// create the adaptors to manage the pipeline in multi-threaded mode
	const size_t buffer_size = 1;
	Adaptor_1_to_n adp_1_to_1_0(params.osf * 2 * params.pl_frame_size,
	                            typeid(float),
	                            buffer_size,
	                            active_waiting,
	                            params.n_frames);
	Adaptor_1_to_n adp_1_to_1_1(params.osf * 2 * params.pl_frame_size,
	                            typeid(float),
	                            buffer_size,
	                            active_waiting,
	                            params.n_frames);
	Adaptor_1_to_n adp_1_to_1_2({(size_t)params.osf * 2 * params.pl_frame_size, (size_t)params.osf * 2 * params.pl_frame_size},
	                            {typeid(float), typeid(int32_t)},
	                            buffer_size,
	                            active_waiting,
	                            params.n_frames);
	Adaptor_1_to_n adp_1_to_1_3(2 * params.pl_frame_size,
	                            typeid(float),
	                            buffer_size,
	                            active_waiting,
	                            params.n_frames);
	Adaptor_1_to_n adp_1_to_1_4(2 * params.pl_frame_size,
	                            typeid(float),
	                            buffer_size,
	                            active_waiting,
	                            params.n_frames);
	Adaptor_1_to_n adp_1_to_n  ({(size_t)2 * params.N_xfec_frame, (size_t)2 * params.N_ldpc / params.bps},
	                            {typeid(float), typeid(float)},
	                            buffer_size,
	                            active_waiting,
	                            params.n_frames);
	Adaptor_n_to_1 adp_n_to_1  ({(size_t)1, (size_t)1, (size_t)params.K_bch},
	                            {typeid(int), typeid(int), typeid(int)},
	                            buffer_size,
	                            active_waiting,
	                            params.n_frames);
#endif /* MULTI_THREADED */

	// add custom name to some modules
	LDPC_decoder ->set_custom_name("LDPC Decoder");
	BCH_decoder  ->set_custom_name("BCH Decoder" );
	sync_lr      ->set_custom_name("L&R F Syn"   );
	sync_fine_pf ->set_custom_name("Fine P/F Syn");
	sync_timing  ->set_custom_name("Gardner Syn" );
	sync_frame   ->set_custom_name("Frame Syn"   );
	matched_flt  ->set_custom_name("Matched Flt" );
	sync_coarse_f->set_custom_name("Coarse_Synch");
	sync_step_mf ->set_custom_name("MF Synch"    );
#ifdef MULTI_THREADED
	adp_1_to_1_0 . set_custom_name("Adp_1_to_1_0");
	adp_1_to_1_1 . set_custom_name("Adp_1_to_1_1");
	adp_1_to_1_2 . set_custom_name("Adp_1_to_1_2");
	adp_1_to_1_3 . set_custom_name("Adp_1_to_1_3");
	adp_1_to_1_4 . set_custom_name("Adp_1_to_1_4");
	adp_1_to_n   . set_custom_name("Adp_1_to_n"  );
	adp_n_to_1   . set_custom_name("Adp_n_to_1"  );
#endif /* MULTI_THREADED */

	// manage noise
	tools::Sigma<> noise_fake(1.f);
	modem->set_noise(noise_fake);
	tools::Sigma<> noise_est;
	estimator->set_noise(noise_est);

	// fill the list of modules
	std::vector<const Module*> modules;
	modules = { /* standard modules */
	            bb_scrambler.get(), BCH_decoder     .get(), source         .get(), LDPC_decoder,
	            itl_rx      .get(), modem           .get(), framer         .get(), pl_scrambler.get(),
	            monitor     .get(), freq_shift      .get(), sync_lr        .get(), sync_fine_pf.get(),
	            radio       .get(), sync_frame      .get(), sync_coarse_f  .get(), matched_flt .get(),
	            sync_timing .get(), sync_step_mf    .get(), mult_agc       .get(), sink        .get(),
	            estimator   .get(), front_agc       .get(),
	            /* probes */
	            prb_sfm_del .get(), prb_sfm_flg     .get(), prb_sfm_tri    .get(), prb_stm_del .get(),
	            prb_frq_coa .get(), prb_frq_lr      .get(), prb_frq_fin    .get(), prb_noise_es.get(),
	            prb_noise_eb.get(), prb_decstat_ldpc.get(), prb_decstat_bch.get(), prb_bfer_be .get(),
	            prb_bfer_fe .get(), prb_bfer_ber    .get(), prb_bfer_fer   .get(), prb_thr_thr .get(),
	            prb_thr_lat .get(), prb_thr_time    .get(), prb_fra_id     .get(), prb_stm_uff .get(),
	            prb_rad_ovf .get(), prb_rad_seq     .get(), prb_thr_thr    .get(),
#ifdef MULTI_THREADED
	            /* adaptors */
	            &adp_1_to_1_0, &adp_1_to_1_1, &adp_1_to_1_2, &adp_1_to_1_3,
	            &adp_1_to_1_4, &adp_1_to_n  , &adp_n_to_1
#endif /* MULTI_THREADED */
	          };

	// configuration of the module tasks
	for (auto& m : modules) for (auto& ta : m->tasks)
	{
		ta->set_autoalloc      (true              ); // enable the automatic allocation of the data in the tasks
		ta->set_debug          (params.debug      ); // disable the debug mode
		ta->set_debug_limit    (params.debug_limit); // display only the 16 first bits if the debug mode is enabled
		ta->set_debug_precision(8                 );
		ta->set_stats          (params.stats      ); // enable the statistics

		if (!ta->is_debug() && !ta->is_stats())
			ta->set_fast(true);
	}

	// execute the source once
	(*source)[src::tsk::generate].exec();

#ifdef MULTI_THREADED
	// parallel chain
	(*modem       )[mdm::sck::demodulate_wg::H_N ].bind(  adp_1_to_n   [adp::sck::pull_n       ::out1  ]);
	(*modem       )[mdm::sck::demodulate_wg::Y_N1].bind(  adp_1_to_n   [adp::sck::pull_n       ::out2  ]);
	(*itl_rx      )[itl::sck::deinterleave ::itl ].bind((*modem       )[mdm::sck::demodulate_wg::Y_N2  ]);
	(*LDPC_decoder)[dec::sck::decode_siho  ::Y_N ].bind((*itl_rx      )[itl::sck::deinterleave ::nat   ]);
	(*BCH_decoder )[dec::sck::decode_hiho  ::Y_N ].bind((*LDPC_decoder)[dec::sck::decode_siho  ::V_K   ]);
	(*bb_scrambler)[scr::sck::descramble   ::Y_N1].bind((*BCH_decoder )[dec::sck::decode_hiho  ::V_K   ]);
	  adp_n_to_1   [adp::sck::push_n       ::in1 ].bind((*LDPC_decoder)[dec::sck::decode_siho  ::status]);
	  adp_n_to_1   [adp::sck::push_n       ::in2 ].bind((*BCH_decoder )[dec::sck::decode_hiho  ::status]);
	  adp_n_to_1   [adp::sck::push_n       ::in3 ].bind((*bb_scrambler)[scr::sck::descramble   ::Y_N2  ]);

	auto start_clone = std::chrono::system_clock::now();
	std::cout << "Cloning the modules of the parallel chain... ";
	std::cout.flush();
	tools::Chain chain_stage6_parallel(adp_1_to_n[adp::tsk::pull_n],
	                                   adp_n_to_1[adp::tsk::push_n],
	                                   24,
	                                   thread_pinnig,
	                                   { 12, 24, 25, 26, 27,
	                                     28, 29, 13, 30, 14,
	                                     31, 15, 32, 16, 33,
	                                     17, 34, 18, 35, 19,
	                                     36, 20, 37, 21, 38,
	                                     22, 39, 23, 39, 40,
	                                     41, 42, 43, 44, 45,
	                                     46, 47 },
	                                   true); // 'false' results in an error because of the clones of the adaptors...
	if (enable_logs)
	{
		std::ofstream f("chain_transmission_6.dot");
		chain_stage6_parallel.export_dot(f);
	}
	auto end_clone = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds_clone = end_clone - start_clone;
	std::cout << "Done (" << elapsed_seconds_clone.count() << "s)." << std::endl;

	if (thread_pinnig)
		tools::Thread_pinning::pin(0);
#endif /* MULTI_THREADED */

	// ================================================================================================================
	// WAITING PHASE ==================================================================================================
	// ================================================================================================================
	auto start_waiting = std::chrono::system_clock::now();
	std::cout << "Waiting phase... ";
	std::cout.flush();

	(*front_agc   )[mlt::sck::imultiply  ::X_N  ].bind((*radio       )[rad::sck::receive    ::Y_N1]);
	(*sync_step_mf)[smf::sck::synchronize::X_N1 ].bind((*front_agc   )[mlt::sck::imultiply  ::Z_N ]);
	(*sync_step_mf)[smf::sck::synchronize::DEL  ].bind((*sync_frame  )[sfm::sck::synchronize::DEL ]);
	(*sync_timing )[stm::sck::extract    ::B_N1 ].bind((*sync_step_mf)[smf::sck::synchronize::B_N1]);
	(*sync_timing )[stm::sck::extract    ::Y_N1 ].bind((*sync_step_mf)[smf::sck::synchronize::Y_N1]);
	(*mult_agc    )[mlt::sck::imultiply  ::X_N  ].bind((*sync_timing )[stm::sck::extract    ::Y_N2]);
	(*sync_frame  )[sfm::sck::synchronize::X_N1 ].bind((*mult_agc    )[mlt::sck::imultiply  ::Z_N ]);

	// add probes
	(*prb_rad_ovf )[prb::sck::probe::in].bind((*radio       )[rad::sck::receive    ::OVF   ]);
	(*prb_rad_seq )[prb::sck::probe::in].bind((*radio       )[rad::sck::receive    ::SEQ   ]);
	(*prb_frq_coa )[prb::sck::probe::in].bind((*sync_step_mf)[smf::sck::synchronize::FRQ   ]);
	(*prb_stm_del )[prb::sck::probe::in].bind((*sync_step_mf)[smf::sck::synchronize::MU    ]);
	(*prb_stm_uff )[prb::sck::probe::in].bind((*sync_timing )[stm::sck::extract    ::UFW   ]);
	(*prb_sfm_del )[prb::sck::probe::in].bind((*sync_frame  )[sfm::sck::synchronize::DEL   ]);
	(*prb_sfm_tri )[prb::sck::probe::in].bind((*sync_frame  )[sfm::sck::synchronize::TRI   ]);
	(*prb_sfm_flg )[prb::sck::probe::in].bind((*sync_frame  )[sfm::sck::synchronize::FLG   ]);
	(*prb_thr_lat )[prb::sck::probe::in].bind((*sync_frame  )[sfm::sck::synchronize::status]);
	(*prb_thr_time)[prb::sck::probe::in].bind((*sync_frame  )[sfm::sck::synchronize::status]);
	(*prb_fra_id  )[prb::sck::probe::in].bind((*sync_frame  )[sfm::sck::synchronize::status]);

	tools::Chain chain_waiting_and_learning_1_2((*radio)[rad::tsk::receive]);
	if (enable_logs)
	{
		std::ofstream fs1("chain_waiting_and_learning_1_2.dot");
		chain_waiting_and_learning_1_2.export_dot(fs1);
	}

	// display the legend in the terminal
	std::ofstream waiting_stats("waiting_stats.txt");
	waiting_stats << "#################" << std::endl;
	waiting_stats << "# WAITING PHASE #" << std::endl;
	waiting_stats << "#################" << std::endl;
	terminal_stats.legend(waiting_stats);

#ifdef DVBS2O_LINK_UHD
	const int radio_flush_period = params.n_frames * 100;
	auto radio_usrp = reinterpret_cast<Radio_USRP<>*>(radio.get());
#endif
	sync_coarse_f->set_PLL_coeffs(1, 1/std::sqrt(2.0), 1e-4);
	prb_thr_thr ->reset();
	prb_thr_lat ->reset();
	prb_thr_time->reset();
	chain_waiting_and_learning_1_2.exec([&](const std::vector<int>& statuses)
	{
		const auto m = prb_fra_id->get_occurrences();
		if (statuses.back() != status_t::SKIPPED)
			terminal_stats.temp_report(waiting_stats);
		else if (enable_logs)
			std::clog << rang::tag::warning << "Chain aborted! (waiting phase, m = " << m << ")" << std::endl;
#ifdef DVBS2O_LINK_UHD
		if (radio_usrp != nullptr && m % radio_flush_period == 0 && !sync_frame->get_packet_flag())
			radio_usrp->flush();
#endif
		return sync_frame->get_packet_flag();
	});

#ifdef DVBS2O_LINK_UHD
	if (radio_usrp != nullptr)
		radio_usrp->flush();
#endif
	sync_step_mf->reset();
	sync_frame  ->reset();
	sync_timing ->reset();
	sync_frame  ->reset();
	prb_fra_id  ->reset();

	auto end_waiting = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds_waiting = end_waiting - start_waiting;
	std::cout << "Done (" << elapsed_seconds_waiting.count() << "s)." << std::endl;

	// ================================================================================================================
	// LEARNING PHASE 1 & 2 ===========================================================================================
	// ================================================================================================================
	auto start_learning = std::chrono::system_clock::now();
	std::cout << "Learning phase... ";
	std::cout.flush();

	// display the legend in the terminal
	std::ofstream stats_file("stats.txt");
	stats_file << "####################" << std::endl;
	stats_file << "# LEARNING PHASE 1 #" << std::endl;
	stats_file << "####################" << std::endl;
	terminal_stats.legend(stats_file);

	int limit = 150;
	sync_coarse_f->set_PLL_coeffs(1, 1/std::sqrt(2.0), 1e-4);
	prb_thr_thr ->reset();
	prb_thr_lat ->reset();
	prb_thr_time->reset();
	chain_waiting_and_learning_1_2.exec([&](const std::vector<int>& statuses)
	{
		const auto m = prb_fra_id->get_occurrences();
		if (statuses.back() != status_t::SKIPPED)
			terminal_stats.temp_report(stats_file);
		else if (enable_logs)
			std::clog << rang::tag::warning << "Chain aborted! (learning phase 1&2, m = " << m << ")" << std::endl;

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

	// ================================================================================================================
	// LEARNING PHASE 3 ===============================================================================================
	// ================================================================================================================
	(*radio       )[rad::sck::receive    ::Y_N1 ].reset();
	(*radio       )[rad::sck::receive    ::OVF  ].reset();
	(*radio       )[rad::sck::receive    ::SEQ  ].reset();
	(*front_agc   )[mlt::sck::imultiply  ::X_N  ].reset();
	(*front_agc   )[mlt::sck::imultiply  ::Z_N  ].reset();
	(*sync_step_mf)[smf::sck::synchronize::X_N1 ].reset();
	(*sync_frame  )[sfm::sck::synchronize::DEL  ].reset();
	(*sync_step_mf)[smf::sck::synchronize::DEL  ].reset();
	(*sync_step_mf)[smf::sck::synchronize::B_N1 ].reset();
	(*sync_timing )[stm::sck::extract    ::B_N1 ].reset();
	(*sync_timing )[stm::sck::extract    ::UFW  ].reset();
	(*sync_step_mf)[smf::sck::synchronize::Y_N1 ].reset();
	(*sync_timing )[stm::sck::extract    ::Y_N1 ].reset();
	(*sync_timing )[stm::sck::extract    ::Y_N2 ].reset();
	(*mult_agc    )[mlt::sck::imultiply  ::X_N  ].reset();
	(*mult_agc    )[mlt::sck::imultiply  ::Z_N  ].reset();
	(*sync_frame  )[sfm::sck::synchronize::X_N1 ].reset();
	(*sync_frame  )[sfm::sck::synchronize::Y_N2 ].reset();

	(*front_agc    )[mlt::sck::imultiply  ::X_N ].bind((*radio        )[rad::sck::receive    ::Y_N1]);
	(*sync_coarse_f)[sfc::sck::synchronize::X_N1].bind((*front_agc    )[mlt::sck::imultiply  ::Z_N ]);
	(*matched_flt  )[flt::sck::filter     ::X_N1].bind((*sync_coarse_f)[sfc::sck::synchronize::Y_N2]);
	(*sync_timing  )[stm::sck::synchronize::X_N1].bind((*matched_flt  )[flt::sck::filter     ::Y_N2]);
	(*sync_timing  )[stm::sck::extract    ::B_N1].bind((*sync_timing  )[stm::sck::synchronize::B_N1]);
	(*sync_timing  )[stm::sck::extract    ::Y_N1].bind((*sync_timing  )[stm::sck::synchronize::Y_N1]);
	(*mult_agc     )[mlt::sck::imultiply  ::X_N ].bind((*sync_timing  )[stm::sck::extract    ::Y_N2]);
	(*sync_frame   )[sfm::sck::synchronize::X_N1].bind((*mult_agc     )[mlt::sck::imultiply  ::Z_N ]);
	(*pl_scrambler )[scr::sck::descramble ::Y_N1].bind((*sync_frame   )[sfm::sck::synchronize::Y_N2]);
	(*sync_lr      )[sff::sck::synchronize::X_N1].bind((*pl_scrambler )[scr::sck::descramble ::Y_N2]);
	(*sync_fine_pf )[sff::sck::synchronize::X_N1].bind((*sync_lr      )[sff::sck::synchronize::Y_N2]);

	// add probes
	(*prb_rad_ovf )[prb::sck::probe      ::in    ].reset();
	(*prb_rad_seq )[prb::sck::probe      ::in    ].reset();
	(*prb_frq_coa )[prb::sck::probe      ::in    ].reset();
	(*prb_stm_del )[prb::sck::probe      ::in    ].reset();
	(*prb_stm_uff )[prb::sck::probe      ::in    ].reset();
	(*prb_sfm_del )[prb::sck::probe      ::in    ].reset();
	(*prb_sfm_tri )[prb::sck::probe      ::in    ].reset();
	(*prb_sfm_flg )[prb::sck::probe      ::in    ].reset();
	(*sync_step_mf)[smf::sck::synchronize::FRQ   ].reset();
	(*sync_step_mf)[smf::sck::synchronize::MU    ].reset();
	(*sync_frame  )[sfm::sck::synchronize::DEL   ].reset();
	(*sync_frame  )[sfm::sck::synchronize::TRI   ].reset();
	(*sync_frame  )[sfm::sck::synchronize::FLG   ].reset();
	(*prb_thr_lat )[prb::sck::probe      ::in    ].reset();
	(*prb_thr_time)[prb::sck::probe      ::in    ].reset();
	(*sync_frame  )[sfm::sck::synchronize::status].reset();
	(*sync_frame  )[sfm::sck::synchronize::status].reset();
	(*prb_fra_id  )[prb::sck::probe      ::in    ].reset();
	(*sync_frame  )[sfm::sck::synchronize::status].reset();

	(*prb_rad_ovf )[prb::sck::probe::in].bind((*radio        )[rad::sck::receive    ::OVF   ]);
	(*prb_rad_seq )[prb::sck::probe::in].bind((*radio        )[rad::sck::receive    ::SEQ   ]);
	(*prb_frq_coa )[prb::sck::probe::in].bind((*sync_coarse_f)[sfc::sck::synchronize::FRQ   ]);
	(*prb_stm_del )[prb::sck::probe::in].bind((*sync_timing  )[stm::sck::synchronize::MU    ]);
	(*prb_stm_uff )[prb::sck::probe::in].bind((*sync_timing  )[stm::sck::extract    ::UFW   ]);
	(*prb_sfm_del )[prb::sck::probe::in].bind((*sync_frame   )[sfm::sck::synchronize::DEL   ]);
	(*prb_sfm_tri )[prb::sck::probe::in].bind((*sync_frame   )[sfm::sck::synchronize::TRI   ]);
	(*prb_sfm_flg )[prb::sck::probe::in].bind((*sync_frame   )[sfm::sck::synchronize::FLG   ]);
	(*prb_frq_lr  )[prb::sck::probe::in].bind((*sync_lr      )[sff::sck::synchronize::FRQ   ]);
	(*prb_frq_fin )[prb::sck::probe::in].bind((*sync_fine_pf )[sff::sck::synchronize::FRQ   ]);
	(*prb_thr_lat )[prb::sck::probe::in].bind((*sync_fine_pf )[sff::sck::synchronize::status]);
	(*prb_thr_time)[prb::sck::probe::in].bind((*sync_fine_pf )[sff::sck::synchronize::status]);
	(*prb_fra_id  )[prb::sck::probe::in].bind((*sync_fine_pf )[sff::sck::synchronize::status]);

	tools::Chain chain_learning_3((*radio)[rad::tsk::receive]);
	if (enable_logs)
	{
		std::ofstream fs2("chain_learning_3.dot");
		chain_learning_3.export_dot(fs2);
	}

	stats_file << "####################" << std::endl;
	stats_file << "# LEARNING PHASE 3 #" << std::endl;
	stats_file << "####################" << std::endl;
	terminal_stats.legend(stats_file);

	limit = prb_fra_id->get_occurrences() + 200;
	prb_thr_thr->reset();
	prb_thr_lat->reset();
	chain_learning_3.exec([&](const std::vector<int>& statuses)
	{
		const auto m = prb_fra_id->get_occurrences();
		if (statuses.back() != status_t::SKIPPED)
			terminal_stats.temp_report(stats_file);
		else if (enable_logs)
			std::clog << rang::tag::warning << "Chain aborted! (learning phase 3, m = " << m << ")" << std::endl;
		return m >= limit;
	});

	auto end_learning = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds_learning = end_learning - start_learning;
	std::cout << "Done (" << elapsed_seconds_learning.count() << "s)." << std::endl;

	// reset the stats of the tasks
	for (auto& m : modules)
		for (auto& ta : m->tasks)
			ta->reset();

	// ================================================================================================================
	// TRANSMISSION PHASE =============================================================================================
	// ================================================================================================================
	// allocate reporters to display results in the terminal
	tools::Reporter_noise<>      rep_noise( noise_est);
	tools::Reporter_BFER<>       rep_BFER (*monitor  );
	tools::Reporter_throughput<> rep_thr  (*monitor  );

	// allocate a terminal that will display the collected data from the reporters
	tools::Terminal_std terminal({ &rep_noise, &rep_BFER, &rep_thr });

	// display the legend in the terminal
	terminal.legend();

	if (params.ter_freq != std::chrono::nanoseconds(0))
		terminal.start_temp_report(params.ter_freq);

	sync_timing->set_act(true);

	stats_file << "######################" << std::endl;
	stats_file << "# TRANSMISSION PHASE #" << std::endl;
	stats_file << "######################" << std::endl;
	terminal_stats.legend(stats_file);

#ifdef MULTI_THREADED
	(*radio        )[rad::sck::receive    ::Y_N1].reset();
	(*front_agc    )[mlt::sck::imultiply  ::X_N ].reset();
	(*front_agc    )[mlt::sck::imultiply  ::Z_N ].reset();
	(*sync_coarse_f)[sfc::sck::synchronize::X_N1].reset();
	(*sync_coarse_f)[sfc::sck::synchronize::Y_N2].reset();
	(*matched_flt  )[flt::sck::filter     ::X_N1].reset();
	(*matched_flt  )[flt::sck::filter     ::Y_N2].reset();
	(*sync_timing  )[stm::sck::synchronize::X_N1].reset();
	(*sync_timing  )[stm::sck::synchronize::B_N1].reset();
	(*sync_timing  )[stm::sck::synchronize::Y_N1].reset();
	(*sync_timing  )[stm::sck::extract    ::B_N1].reset();
	(*sync_timing  )[stm::sck::extract    ::Y_N1].reset();
	(*sync_timing  )[stm::sck::extract    ::Y_N2].reset();
	(*mult_agc     )[mlt::sck::imultiply  ::X_N ].reset();
	(*mult_agc     )[mlt::sck::imultiply  ::Z_N ].reset();
	(*sync_frame   )[sfm::sck::synchronize::X_N1].reset();
	(*sync_frame   )[sfm::sck::synchronize::Y_N2].reset();
	(*pl_scrambler )[scr::sck::descramble ::Y_N1].reset();
	(*pl_scrambler )[scr::sck::descramble ::Y_N2].reset();
	(*sync_lr      )[sff::sck::synchronize::X_N1].reset();
	(*sync_lr      )[sff::sck::synchronize::Y_N2].reset();
	(*sync_fine_pf )[sff::sck::synchronize::X_N1].reset();
	(*sync_fine_pf )[sff::sck::synchronize::Y_N2].reset();

	  adp_1_to_1_0  [adp::sck::push_1     ::in1 ].bind((*radio        )[rad::sck::receive    ::Y_N1]);
	(*front_agc    )[mlt::sck::imultiply  ::X_N ].bind(  adp_1_to_1_0  [adp::sck::pull_n     ::out1]);
	(*sync_coarse_f)[sfc::sck::synchronize::X_N1].bind((*front_agc    )[mlt::sck::imultiply  ::Z_N ]);
	(*matched_flt  )[flt::sck::filter     ::X_N1].bind((*sync_coarse_f)[sfc::sck::synchronize::Y_N2]);
	  adp_1_to_1_1  [adp::sck::push_1     ::in1 ].bind((*matched_flt  )[flt::sck::filter     ::Y_N2]);
	(*sync_timing  )[stm::sck::synchronize::X_N1].bind(  adp_1_to_1_1  [adp::sck::pull_n     ::out1]);
	  adp_1_to_1_2  [adp::sck::push_1     ::in1 ].bind((*sync_timing  )[stm::sck::synchronize::Y_N1]);
	  adp_1_to_1_2  [adp::sck::push_1     ::in2 ].bind((*sync_timing  )[stm::sck::synchronize::B_N1]);
	(*sync_timing  )[stm::sck::extract    ::B_N1].bind(  adp_1_to_1_2  [adp::sck::pull_n     ::out2]);
	(*sync_timing  )[stm::sck::extract    ::Y_N1].bind(  adp_1_to_1_2  [adp::sck::pull_n     ::out1]);
	(*mult_agc     )[mlt::sck::imultiply  ::X_N ].bind((*sync_timing  )[stm::sck::extract    ::Y_N2]);
	(*sync_frame   )[sfm::sck::synchronize::X_N1].bind((*mult_agc     )[mlt::sck::imultiply  ::Z_N ]);
	  adp_1_to_1_3  [adp::sck::push_1     ::in1 ].bind((*sync_frame   )[sfm::sck::synchronize::Y_N2]);
	(*pl_scrambler )[scr::sck::descramble ::Y_N1].bind(  adp_1_to_1_3  [adp::sck::pull_n     ::out1]);
	(*sync_lr      )[sff::sck::synchronize::X_N1].bind((*pl_scrambler )[scr::sck::descramble ::Y_N2]);
	(*sync_fine_pf )[sff::sck::synchronize::X_N1].bind((*sync_lr      )[sff::sck::synchronize::Y_N2]);
	  adp_1_to_1_4  [adp::sck::push_1     ::in1 ].bind((*sync_fine_pf )[sff::sck::synchronize::Y_N2]);
	(*framer       )[frm::sck::remove_plh ::Y_N1].bind(  adp_1_to_1_4  [adp::sck::pull_n     ::out1]);
	  adp_1_to_n    [adp::sck::push_1     ::in1 ].bind((*estimator    )[est::sck::rescale    ::H_N ]);
	(*estimator    )[est::sck::rescale    ::X_N ].bind((*framer       )[frm::sck::remove_plh ::Y_N2]);
	  adp_1_to_n    [adp::sck::push_1     ::in2 ].bind((*estimator    )[est::sck::rescale    ::Y_N ]);
	// parallel chain (modem / decoder LDPC / decoder BCH)
	(*monitor      )[mnt::sck::check_errors::U  ].bind((*source       )[src::sck::generate   ::U_K ]);
	(*monitor      )[mnt::sck::check_errors::V  ].bind(  adp_n_to_1    [adp::sck::pull_1     ::out3]);
	(*sink         )[snk::sck::send        ::V  ].bind(  adp_n_to_1    [adp::sck::pull_1     ::out3]);

	// add probes
	(*prb_thr_lat )[prb::sck::probe      ::in    ].reset();
	(*prb_thr_time)[prb::sck::probe      ::in    ].reset();
	(*sync_fine_pf)[sff::sck::synchronize::status].reset();
	(*sync_fine_pf)[sff::sck::synchronize::status].reset();
	(*prb_fra_id  )[prb::sck::probe      ::in    ].reset();
	(*sync_fine_pf)[sff::sck::synchronize::status].reset();

	const int high_priority = 0;
	(*prb_decstat_ldpc)[prb::sck::probe::in].bind(  adp_n_to_1[adp::sck::pull_1      ::out1  ], high_priority);
	(*prb_decstat_bch )[prb::sck::probe::in].bind(  adp_n_to_1[adp::sck::pull_1      ::out2  ], high_priority);
	(*prb_thr_thr     )[prb::sck::probe::in].bind(  adp_n_to_1[adp::sck::pull_1      ::out3  ], high_priority);
	(*prb_thr_lat     )[prb::sck::probe::in].bind((*sink     )[snk::sck::send        ::status], high_priority);
	(*prb_thr_time    )[prb::sck::probe::in].bind((*sink     )[snk::sck::send        ::status], high_priority);
	(*prb_noise_es    )[prb::sck::probe::in].bind((*estimator)[est::sck::rescale     ::Es_N0 ], high_priority);
	(*prb_noise_eb    )[prb::sck::probe::in].bind((*estimator)[est::sck::rescale     ::Eb_N0 ], high_priority);
	(*prb_bfer_be     )[prb::sck::probe::in].bind((*monitor  )[mnt::sck::check_errors::BE    ], high_priority);
	(*prb_bfer_fe     )[prb::sck::probe::in].bind((*monitor  )[mnt::sck::check_errors::FE    ], high_priority);
	(*prb_bfer_ber    )[prb::sck::probe::in].bind((*monitor  )[mnt::sck::check_errors::BER   ], high_priority);
	(*prb_bfer_fer    )[prb::sck::probe::in].bind((*monitor  )[mnt::sck::check_errors::FER   ], high_priority);
	(*prb_fra_id      )[prb::sck::probe::in].bind((*sink     )[snk::sck::send        ::status], high_priority);

	// create a chain per pipeline stage
	tools::Chain chain_stage0((*radio       )[rad::tsk::receive], adp_1_to_1_0[adp::tsk::push_1], 1, thread_pinnig, { 2 });
	tools::Chain chain_stage1(  adp_1_to_1_0 [adp::tsk::pull_n ], adp_1_to_1_1[adp::tsk::push_1], 1, thread_pinnig, { 3 });
	tools::Chain chain_stage2(  adp_1_to_1_1 [adp::tsk::pull_n ], adp_1_to_1_2[adp::tsk::push_1], 1, thread_pinnig, { 4 });
	tools::Chain chain_stage3(  adp_1_to_1_2 [adp::tsk::pull_n ], adp_1_to_1_3[adp::tsk::push_1], 1, thread_pinnig, { 5 });
	tools::Chain chain_stage4(  adp_1_to_1_3 [adp::tsk::pull_n ], adp_1_to_1_4[adp::tsk::push_1], 1, thread_pinnig, { 6 });
	tools::Chain chain_stage5(  adp_1_to_1_4 [adp::tsk::pull_n ], adp_1_to_n  [adp::tsk::push_1], 1, thread_pinnig, { 7 });
	tools::Chain chain_stage7(  adp_n_to_1   [adp::tsk::pull_1 ],                                 1, thread_pinnig, { 8 });

	std::vector<tools::Chain*> chain_stages = { &chain_stage0,          &chain_stage1, &chain_stage2,
	                                            &chain_stage3,          &chain_stage4, &chain_stage5,
	                                            &chain_stage6_parallel, &chain_stage7,                };

	// dump the chains in dot format
	for (size_t cs = 0; cs < chain_stages.size() && enable_logs; cs++)
	{
		std::ofstream fs("chain_transmission_" + std::to_string(cs) + ".dot");
		chain_stages[cs]->export_dot(fs);
	}

	// function to wake up and stop all the threads
	auto stop_threads = [&chain_stages]()
	{
		for (auto &cs : chain_stages)
			for (auto &m : cs->get_modules<tools::Interface_waiting>())
				m->cancel_waiting();
	};

	// start the pipeline stages in separated threads
	std::vector<std::thread> threads;
	prb_thr_thr->reset();
	prb_thr_lat->reset();
	for (size_t s = 0; s < chain_stages.size(); s++)
	{
		auto cs = chain_stages[s];
		threads.push_back(std::thread([&prb_fra_id, &prb_thr_the, s, cs, &terminal_stats, &stats_file, &stop_threads]() {
			cs->exec([&prb_fra_id, &prb_thr_the, s, &terminal_stats, &stats_file](const std::vector<int>& statuses)
			{
				if (s == 3 && statuses.back() == status_t::SKIPPED && enable_logs)
					std::clog << std::endl << rang::tag::warning << "Chain aborted! (transmission phase, stage = " << s
					          << ", m = " << prb_fra_id->get_occurrences() << ")" << std::endl;
				if (s == 7)
				{
					(*prb_thr_the)[prb::tsk::probe].exec();
					terminal_stats.temp_report(stats_file);
				}
				return tools::Terminal::is_interrupt();
			});
			stop_threads();
		}));

	}

	// wait all the pipeline threads here
	for (auto &t : threads)
		t.join();
#else
	(*framer      )[frm::sck::remove_plh   ::Y_N1].bind((*sync_fine_pf)[sff::sck::synchronize  ::Y_N2]);
	(*estimator   )[est::sck::rescale      ::X_N ].bind((*framer      )[frm::sck::remove_plh   ::Y_N2]);
	(*modem       )[mdm::sck::demodulate_wg::H_N ].bind((*estimator   )[est::sck::rescale      ::H_N ]);
	(*modem       )[mdm::sck::demodulate_wg::Y_N1].bind((*estimator   )[est::sck::rescale      ::Y_N ]);
	(*itl_rx      )[itl::sck::deinterleave ::itl ].bind((*modem       )[mdm::sck::demodulate_wg::Y_N2]);
	(*LDPC_decoder)[dec::sck::decode_siho  ::Y_N ].bind((*itl_rx      )[itl::sck::deinterleave ::nat ]);
	(*BCH_decoder )[dec::sck::decode_hiho  ::Y_N ].bind((*LDPC_decoder)[dec::sck::decode_siho  ::V_K ]);
	(*bb_scrambler)[scr::sck::descramble   ::Y_N1].bind((*BCH_decoder )[dec::sck::decode_hiho  ::V_K ]);
	(*monitor     )[mnt::sck::check_errors ::U   ].bind((*source      )[src::sck::generate     ::U_K ]);
	(*monitor     )[mnt::sck::check_errors ::V   ].bind((*bb_scrambler)[scr::sck::descramble   ::Y_N2]);
	(*sink        )[snk::sck::send         ::V   ].bind((*bb_scrambler)[scr::sck::descramble   ::Y_N2]);

	// add probes
	(*prb_thr_lat )[prb::sck::probe      ::in    ].reset();
	(*prb_thr_time)[prb::sck::probe      ::in    ].reset();
	(*sync_fine_pf)[sff::sck::synchronize::status].reset();
	(*sync_fine_pf)[sff::sck::synchronize::status].reset();
	(*prb_fra_id  )[prb::sck::probe      ::in    ].reset();
	(*sync_frame  )[sfm::sck::synchronize::status].reset();

	(*prb_decstat_ldpc)[prb::sck::probe::in].bind((*LDPC_decoder)[dec::sck::decode_siho ::status]);
	(*prb_decstat_bch )[prb::sck::probe::in].bind((*BCH_decoder )[dec::sck::decode_hiho ::status]);
	(*prb_thr_thr     )[prb::sck::probe::in].bind((*BCH_decoder )[dec::sck::decode_hiho ::V_K   ]);
	(*prb_thr_lat     )[prb::sck::probe::in].bind((*sink        )[snk::sck::send        ::status]);
	(*prb_thr_time    )[prb::sck::probe::in].bind((*sink        )[snk::sck::send        ::status]);
	(*prb_noise_es    )[prb::sck::probe::in].bind((*estimator   )[est::sck::rescale     ::Es_N0 ]);
	(*prb_noise_eb    )[prb::sck::probe::in].bind((*estimator   )[est::sck::rescale     ::Eb_N0 ]);
	(*prb_bfer_be     )[prb::sck::probe::in].bind((*monitor     )[mnt::sck::check_errors::BE    ]);
	(*prb_bfer_fe     )[prb::sck::probe::in].bind((*monitor     )[mnt::sck::check_errors::FE    ]);
	(*prb_bfer_ber    )[prb::sck::probe::in].bind((*monitor     )[mnt::sck::check_errors::BER   ]);
	(*prb_bfer_fer    )[prb::sck::probe::in].bind((*monitor     )[mnt::sck::check_errors::FER   ]);
	(*prb_fra_id      )[prb::sck::probe::in].bind((*sink        )[snk::sck::send        ::status]);

	tools::Chain chain_transmission((*radio)[rad::tsk::receive]);
	if (enable_logs)
	{
		std::ofstream fs3("chain_transmission.dot");
		chain_transmission.export_dot(fs3);
	}

	// start the transmission chain
	prb_thr_thr->reset();
	prb_thr_lat->reset();
	chain_transmission.exec([&prb_fra_id, &terminal_stats, &stats_file](const std::vector<int>& statuses)
	{
		if (statuses.back() != status_t::SKIPPED)
			terminal_stats.temp_report(stats_file);
		else if (enable_logs)
			std::clog << std::endl << rang::tag::warning << "Chain aborted! (transmission phase, m = "
			          << prb_fra_id->get_occurrences() << ")" << std::endl;
		return tools::Terminal::is_interrupt();
	});

	// stop the radio thread
	for (auto &m : chain_transmission.get_modules<tools::Interface_waiting>())
		m->cancel_waiting();
#endif /* MULTI_THREADED */

	// display the performance (BER and FER) in the terminals
	terminal      .final_report(          );
	terminal_stats.final_report(stats_file);

	if (params.stats)
	{
		std::vector<const Module*> modules_stats(modules.size());
		for (size_t m = 0; m < modules.size(); m++)
			modules_stats.push_back(modules[m]);

		const auto ordered = true;
#ifdef MULTI_THREADED
		for (size_t cs = 0; cs < chain_stages.size(); cs++)
		{
			std::cout << "#" << std::endl << "# Chain stage " << cs << " (" << chain_stages[cs]->get_n_threads()
			          << " thread(s)): " << std::endl;
			tools::Stats::show(chain_stages[cs]->get_tasks_per_types(), ordered);
		}
#else
		std::cout << "#" << std::endl << "# Chain sequential (" << chain_transmission.get_n_threads() << " thread(s)): "
		          << std::endl;
		tools::Stats::show(chain_transmission.get_tasks_per_types(), ordered);
#endif /* MULTI_THREADED */
	}

	std::cout << "#" << std::endl;
	std::cout << "# End of the simulation" << std::endl;

#ifdef MULTI_THREADED
	if (thread_pinnig)
		tools::Thread_pinning::destroy();
#endif

	return EXIT_SUCCESS;
}

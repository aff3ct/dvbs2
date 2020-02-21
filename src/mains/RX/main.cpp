#include <chrono>
#include <fstream>
#include <aff3ct.hpp>

#include "Tools/Reporter/Reporter_probe.hpp"
#include "Tools/Reporter/Reporter_probe_decstat.hpp"
#include "Tools/Reporter/Reporter_throughput_DVBS2O.hpp"
#include "Factory/DVBS2O/DVBS2O.hpp"

using namespace aff3ct;

#define MULTI_THREADED // comment this line to disable multi-threaded RX

#ifdef MULTI_THREADED
const bool thread_pinnig = true;
const bool active_waiting = false;
#endif

int main(int argc, char** argv)
{
#ifdef MULTI_THREADED
	if (thread_pinnig)
	{
		tools::Thread_pinning::init();
		// tools::Thread_pinning::set_logs(true);
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
	std::unique_ptr<tools::Constellation<float>> cstl(new tools::Constellation_user<float>(params.constellation_file));
	std::unique_ptr<tools::Interleaver_core<>> itl_core(factory::DVBS2O::build_itl_core<>(params));
	tools::BCH_polynomial_generator<> poly_gen(params.N_bch_unshortened, 12, params.bch_prim_poly);

	// construct modules
	std::unique_ptr<module::Source<>                   > source       (factory::DVBS2O::build_source                  <>(params              ));
	std::unique_ptr<module::Sink<>                     > sink         (factory::DVBS2O::build_sink                    <>(params              ));
	std::unique_ptr<module::Radio<>                    > radio        (factory::DVBS2O::build_radio                   <>(params              ));
	std::unique_ptr<module::Scrambler<>                > bb_scrambler (factory::DVBS2O::build_bb_scrambler            <>(params              ));
	std::unique_ptr<module::Decoder_HIHO<>             > BCH_decoder  (factory::DVBS2O::build_bch_decoder             <>(params, poly_gen    ));
	std::unique_ptr<tools ::Codec_SIHO<>               > LDPC_cdc     (factory::DVBS2O::build_ldpc_cdc                <>(params              ));
	std::unique_ptr<module::Interleaver<float,uint32_t>> itl_rx       (factory::DVBS2O::build_itl<float,uint32_t>       (params, *itl_core   ));
	std::unique_ptr<module::Modem<>                    > modem        (factory::DVBS2O::build_modem                   <>(params, cstl.get()  ));
	std::unique_ptr<module::Multiplier_sine_ccc_naive<>> freq_shift   (factory::DVBS2O::build_freq_shift              <>(params              ));
	std::unique_ptr<module::Synchronizer_frame<>       > sync_frame   (factory::DVBS2O::build_synchronizer_frame      <>(params              ));
	std::unique_ptr<module::Synchronizer_freq_fine<>   > sync_lr      (factory::DVBS2O::build_synchronizer_lr         <>(params              ));
	std::unique_ptr<module::Synchronizer_freq_fine<>   > sync_fine_pf (factory::DVBS2O::build_synchronizer_freq_phase <>(params              ));
	std::unique_ptr<module::Framer<>                   > framer       (factory::DVBS2O::build_framer                  <>(params              ));
	std::unique_ptr<module::Scrambler<float>           > pl_scrambler (factory::DVBS2O::build_pl_scrambler            <>(params              ));
	std::unique_ptr<module::Monitor_BFER<>             > monitor      (factory::DVBS2O::build_monitor                 <>(params              ));
	std::unique_ptr<module::Filter_RRC_ccr_naive<>     > matched_flt  (factory::DVBS2O::build_matched_filter          <>(params              ));
	std::unique_ptr<module::Synchronizer_timing <>     > sync_timing  (factory::DVBS2O::build_synchronizer_timing     <>(params              ));
	std::unique_ptr<module::Multiplier_AGC_cc_naive<>  > front_agc    (factory::DVBS2O::build_channel_agc             <>(params              ));
	std::unique_ptr<module::Multiplier_AGC_cc_naive<>  > mult_agc     (factory::DVBS2O::build_agc_shift               <>(params              ));
	std::unique_ptr<module::Estimator<>                > estimator    (factory::DVBS2O::build_estimator               <>(params              ));
	std::unique_ptr<module::Synchronizer_freq_coarse<> > sync_coarse_f(factory::DVBS2O::build_synchronizer_freq_coarse<>(params              ));
	std::unique_ptr<module::Synchronizer_step_mf_cc<>  > sync_step_mf (factory::DVBS2O::build_synchronizer_step_mf_cc <>(params,
	                                                                                                                     sync_coarse_f.get(),
	                                                                                                                     matched_flt  .get(),
	                                                                                                                     sync_timing  .get() ));
	auto* LDPC_decoder = &LDPC_cdc->get_decoder_siho();

	tools::Reporter_BFER<> rep_BFER_stats(*monitor);
	tools::Reporter_throughput_DVBS2O<> rep_thr_stats(*monitor);

	tools::Reporter_probe rep_sfm_stats("Frame Synchronization", params.n_frames);
	std::unique_ptr<module::Probe<int  >> prb_sfm_del(rep_sfm_stats.create_probe<int  >("DEL", ""));
	std::unique_ptr<module::Probe<int  >> prb_sfm_flg(rep_sfm_stats.create_probe<int  >("FLG", ""));
	std::unique_ptr<module::Probe<float>> prb_sfm_tri(rep_sfm_stats.create_probe<float>("TRI", ""));

	tools::Reporter_probe rep_stm_stats("Timing Synchronization", "Gardner Algorithm", params.n_frames);
	std::unique_ptr<module::Probe<float>> prb_stm_del(rep_stm_stats.create_probe<float>("DEL", "FRAC"));

	tools::Reporter_probe rep_frq_stats("Freq. Synchronization", params.n_frames);
	std::unique_ptr<module::Probe<float>> prb_frq_coa(rep_frq_stats.create_probe<float>("COA", "CFO"));
	std::unique_ptr<module::Probe<float>> prb_frq_lr (rep_frq_stats.create_probe<float>("L&R", "CFO"));
	std::unique_ptr<module::Probe<float>> prb_frq_fin(rep_frq_stats.create_probe<float>("FIN", "CFO"));

	tools::Reporter_probe_decstat rep_decstat_stats("Decoders decoding status", "('1' = fail, '0' = success)", params.n_frames);
	std::unique_ptr<module::Probe<int>> prb_decstat_ldpc(rep_decstat_stats.create_probe<int>("LDPC", ""));
	std::unique_ptr<module::Probe<int>> prb_decstat_bch (rep_decstat_stats.create_probe<int>("BCH", ""));

	std::ios_base::fmtflags ff;
	ff |= std::cout.fixed;
	tools::Reporter_probe rep_noise_stats("Signal Noise Ratio", "(SNR)", params.n_frames);
	std::unique_ptr<module::Probe<float>> prb_noise_es(rep_noise_stats.create_probe<float>("Es/N0", "(dB)", ff, 4));
	std::unique_ptr<module::Probe<float>> prb_noise_eb(rep_noise_stats.create_probe<float>("Eb/N0", "(dB)", ff, 4));

	tools::Terminal_dump terminal_stats({ &rep_sfm_stats,   &rep_stm_stats,  &rep_frq_stats, &rep_decstat_stats,
	                                      &rep_noise_stats, &rep_BFER_stats, &rep_thr_stats                      });

#ifdef MULTI_THREADED
	const size_t buffer_size = 1;
	module::Adaptor_1_to_n adp_1_to_1_0(params.osf * 2 * params.pl_frame_size, typeid(float), buffer_size, active_waiting, params.n_frames);
	module::Adaptor_1_to_n adp_1_to_1_1(params.osf * 2 * params.pl_frame_size, typeid(float), buffer_size, active_waiting, params.n_frames);
	module::Adaptor_1_to_n adp_1_to_1_2({(size_t)params.osf * 2 * params.pl_frame_size, (size_t)params.osf * 2 * params.pl_frame_size}, {typeid(float), typeid(int32_t)}, buffer_size, active_waiting, params.n_frames);
	module::Adaptor_1_to_n adp_1_to_1_3(2 * params.pl_frame_size, typeid(float), buffer_size, active_waiting, params.n_frames);
	module::Adaptor_1_to_n adp_1_to_1_4(2 * params.pl_frame_size, typeid(float), buffer_size, active_waiting, params.n_frames);
	module::Adaptor_1_to_n adp_1_to_n  ({(size_t)2 * params.N_xfec_frame, (size_t)params.N_ldpc}, {typeid(float), typeid(float)}, buffer_size, active_waiting, params.n_frames);
	module::Adaptor_n_to_1 adp_n_to_1  ({(size_t)1, (size_t)1, (size_t)params.K_bch}, {typeid(int), typeid(int), typeid(int)}, buffer_size, active_waiting, params.n_frames);
#endif /* MULTI_THREADED */

	// manage noise
	tools::Sigma<> fake_noise(1.f);
	modem->set_noise(fake_noise);
	tools::Sigma<> noise;
	estimator->set_noise(noise);

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
	adp_1_to_1_0.set_custom_name("Adp_1_to_1_0");
	adp_1_to_1_1.set_custom_name("Adp_1_to_1_1");
	adp_1_to_1_2.set_custom_name("Adp_1_to_1_2");
	adp_1_to_1_3.set_custom_name("Adp_1_to_1_3");
	adp_1_to_1_4.set_custom_name("Adp_1_to_1_4");
	adp_1_to_n  .set_custom_name("Adp_1_to_n"  );
	adp_n_to_1  .set_custom_name("Adp_n_to_1"  );
#endif /* MULTI_THREADED */

	// fill the list of modules
	std::vector<const module::Module*> modules;
	modules = { bb_scrambler    .get(), BCH_decoder     .get(), source         .get(), LDPC_decoder,
	            itl_rx          .get(), modem           .get(), framer         .get(), pl_scrambler.get(),
	            monitor         .get(), freq_shift      .get(), sync_lr        .get(), sync_fine_pf.get(),
	            radio           .get(), sync_frame      .get(), sync_coarse_f  .get(), matched_flt .get(),
	            sync_timing     .get(), sync_step_mf    .get(), mult_agc       .get(), sink        .get(),
	            estimator       .get(), front_agc       .get(),
	            /* probes */
	            prb_sfm_del     .get(), prb_sfm_flg     .get(), prb_sfm_tri    .get(), prb_stm_del .get(),
	            prb_frq_coa     .get(), prb_frq_lr      .get(), prb_frq_fin    .get(), prb_noise_es.get(),
	            prb_noise_eb    .get(), prb_decstat_ldpc.get(), prb_decstat_bch.get(),
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
		ta->set_fast           (false             ); // disable the fast mode
	}

	// exec the source once
	(*source)[module::src::tsk::generate].exec();

	using namespace module;

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
	tools::Chain chain_stage6_parallel(adp_1_to_n[module::adp::tsk::pull_n],
	                                   adp_n_to_1[module::adp::tsk::push_n],
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
	std::ofstream f("chain_stage6_parallel.dot");
	chain_stage6_parallel.export_dot(f);
	auto end_clone = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds_clone = end_clone - start_clone;
	std::cout << "Done (" << elapsed_seconds_clone.count() << "s)." << std::endl;

	if (thread_pinnig)
		tools::Thread_pinning::pin(0);
#endif /* MULTI_THREADED */

	// ================================================================================================================
	// LEARNING PHASE 1 & 2 ===========================================================================================

	auto start_learning = std::chrono::system_clock::now();
	std::cout << "Learning phase... ";
	std::cout.flush();

	(*front_agc   )[mlt::sck::imultiply  ::X_N  ].bind((*radio       )[rad::sck::receive    ::Y_N1 ]);
	(*sync_step_mf)[smf::sck::synchronize::X_N1 ].bind((*front_agc   )[mlt::sck::imultiply  ::Z_N  ]);
	(*sync_step_mf)[smf::sck::synchronize::DEL  ].bind((*sync_frame  )[sfm::sck::synchronize::DEL  ]);
	(*sync_timing )[stm::sck::extract    ::B_N1 ].bind((*sync_step_mf)[smf::sck::synchronize::B_N1 ]);
	(*sync_timing )[stm::sck::extract    ::Y_N1 ].bind((*sync_step_mf)[smf::sck::synchronize::Y_N1 ]);
	(*mult_agc    )[mlt::sck::imultiply  ::X_N  ].bind((*sync_timing )[stm::sck::extract    ::Y_N2 ]);
	(*sync_frame  )[sfm::sck::synchronize::X_N1 ].bind((*mult_agc    )[mlt::sck::imultiply  ::Z_N  ]);

	// add probes
	const int high_priority = 0;
	(*prb_frq_coa)[prb::sck::probe::X_N].bind((*sync_step_mf)[smf::sck::synchronize::FRQ], high_priority);
	(*prb_stm_del)[prb::sck::probe::X_N].bind((*sync_step_mf)[smf::sck::synchronize::MU ], high_priority);
	(*prb_sfm_del)[prb::sck::probe::X_N].bind((*sync_frame  )[sfm::sck::synchronize::DEL], high_priority);
	(*prb_sfm_tri)[prb::sck::probe::X_N].bind((*sync_frame  )[sfm::sck::synchronize::TRI], high_priority);
	(*prb_sfm_flg)[prb::sck::probe::X_N].bind((*sync_frame  )[sfm::sck::synchronize::FLG], high_priority);

	tools::Chain chain_sequential1((*radio)[rad::tsk::receive]);
	std::ofstream fs1("chain_sequential1.dot");
	chain_sequential1.export_dot(fs1);

	// display the legend in the terminal
	std::ofstream stats_file("stats.txt");
	stats_file << "####################" << std::endl;
	stats_file << "# LEARNING PHASE 1 #" << std::endl;
	stats_file << "####################" << std::endl;
	terminal_stats.legend(stats_file);

	int m = 0;
	int limit = 150;
	sync_coarse_f->set_PLL_coeffs(1, 1/std::sqrt(2.0), 1e-4);
	chain_sequential1.exec([&](const std::vector<int>& statuses)
	{
		if (statuses.back() != status_t::SKIPPED)
			terminal_stats.temp_report(stats_file);
		if (limit == 150 && m >= 150)
		{
			stats_file << "####################" << std::endl;
			stats_file << "# LEARNING PHASE 2 #" << std::endl;
			stats_file << "####################" << std::endl;
			terminal_stats.legend(stats_file);
			limit = 300;
			sync_coarse_f->set_PLL_coeffs(1, 1/std::sqrt(2.0), 5e-5);
		}
		const auto stop = m >= limit;
		m += statuses.back() != status_t::SKIPPED ? params.n_frames : 0;
		return stop;
	});

	// ================================================================================================================
	// LEARNING PHASE 3 ===============================================================================================

	(*radio       )[rad::sck::receive    ::Y_N1 ].reset();
	(*front_agc   )[mlt::sck::imultiply  ::X_N  ].reset();
	(*front_agc   )[mlt::sck::imultiply  ::Z_N  ].reset();
	(*sync_step_mf)[smf::sck::synchronize::X_N1 ].reset();
	(*sync_frame  )[sfm::sck::synchronize::DEL  ].reset();
	(*sync_step_mf)[smf::sck::synchronize::DEL  ].reset();
	(*sync_step_mf)[smf::sck::synchronize::B_N1 ].reset();
	(*sync_timing )[stm::sck::extract    ::B_N1 ].reset();
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
	(*prb_frq_coa )[prb::sck::probe      ::X_N].reset();
	(*prb_stm_del )[prb::sck::probe      ::X_N].reset();
	(*prb_sfm_del )[prb::sck::probe      ::X_N].reset();
	(*prb_sfm_tri )[prb::sck::probe      ::X_N].reset();
	(*prb_sfm_flg )[prb::sck::probe      ::X_N].reset();
	(*sync_step_mf)[smf::sck::synchronize::FRQ].reset();
	(*sync_step_mf)[smf::sck::synchronize::MU ].reset();
	(*sync_frame  )[sfm::sck::synchronize::DEL].reset();
	(*sync_frame  )[sfm::sck::synchronize::TRI].reset();
	(*sync_frame  )[sfm::sck::synchronize::FLG].reset();

	(*prb_frq_coa)[prb::sck::probe::X_N].bind((*sync_coarse_f)[sfc::sck::synchronize::FRQ], high_priority);
	(*prb_stm_del)[prb::sck::probe::X_N].bind((*sync_timing  )[stm::sck::synchronize::MU ], high_priority);
	(*prb_sfm_del)[prb::sck::probe::X_N].bind((*sync_frame   )[sfm::sck::synchronize::DEL], high_priority);
	(*prb_sfm_tri)[prb::sck::probe::X_N].bind((*sync_frame   )[sfm::sck::synchronize::TRI], high_priority);
	(*prb_sfm_flg)[prb::sck::probe::X_N].bind((*sync_frame   )[sfm::sck::synchronize::FLG], high_priority);
	(*prb_frq_lr )[prb::sck::probe::X_N].bind((*sync_lr      )[sff::sck::synchronize::FRQ], high_priority);
	(*prb_frq_fin)[prb::sck::probe::X_N].bind((*sync_fine_pf )[sff::sck::synchronize::FRQ], high_priority);

	tools::Chain chain_sequential2((*radio)[rad::tsk::receive]);
	std::ofstream fs2("chain_sequential2.dot");
	chain_sequential2.export_dot(fs2);

	stats_file << "####################" << std::endl;
	stats_file << "# LEARNING PHASE 3 #" << std::endl;
	stats_file << "####################" << std::endl;
	terminal_stats.legend(stats_file);

	chain_sequential2.exec([&](const std::vector<int>& statuses)
	{
		const auto stop = m >= 500;
		if (statuses.back() != status_t::SKIPPED)
			terminal_stats.temp_report(stats_file);
		m += statuses.back() != status_t::SKIPPED ? params.n_frames : 0;
		return stop;
	});

	auto end_learning = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds_learning = end_learning - start_learning;
	std::cout << "Done (" << elapsed_seconds_learning.count() << "s)." << std::endl;

	// reset the stats of the tasks
	for (auto& m : modules)
		for (auto& ta : m->tasks)
			ta->reset();

	// allocate reporters to display results in the terminal
	tools::Reporter_noise<>      rep_noise( noise  );
	tools::Reporter_BFER<>       rep_BFER (*monitor);
	tools::Reporter_throughput<> rep_thr  (*monitor);

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
	  adp_1_to_n    [adp::sck::push_1     ::in1 ].bind((*estimator    )[est::sck::estimate   ::H_N ]);
	(*estimator    )[est::sck::estimate   ::X_N ].bind((*framer       )[frm::sck::remove_plh ::Y_N2]);
	  adp_1_to_n    [adp::sck::push_1     ::in2 ].bind((*framer       )[frm::sck::remove_plh ::Y_N2]);
	// parallel chain (modem / decoder LDPC / decoder BCH)
	(*monitor      )[mnt::sck::check_errors::U  ].bind((*source       )[src::sck::generate   ::U_K ]);
	(*monitor      )[mnt::sck::check_errors::V  ].bind(  adp_n_to_1    [adp::sck::pull_1     ::out3]);
	(*sink         )[snk::sck::send        ::V  ].bind(  adp_n_to_1    [adp::sck::pull_1     ::out3]);

	// add probes
	(*prb_decstat_ldpc)[prb::sck::probe::X_N].bind(  adp_n_to_1[adp::sck::pull_1  ::out1 ], high_priority);
	(*prb_decstat_bch )[prb::sck::probe::X_N].bind(  adp_n_to_1[adp::sck::pull_1  ::out2 ], high_priority);
	(*prb_noise_es    )[prb::sck::probe::X_N].bind((*estimator)[est::sck::estimate::Es_N0], high_priority);
	(*prb_noise_eb    )[prb::sck::probe::X_N].bind((*estimator)[est::sck::estimate::Eb_N0], high_priority);

	// create a chain per pipeline stage
	tools::Chain chain_stage0((*radio       )[rad::tsk::receive],   adp_1_to_1_0 [adp::tsk::push_1], 1, thread_pinnig, { 2 });
	tools::Chain chain_stage1(  adp_1_to_1_0 [adp::tsk::pull_n ],   adp_1_to_1_1 [adp::tsk::push_1], 1, thread_pinnig, { 3 });
	tools::Chain chain_stage2(  adp_1_to_1_1 [adp::tsk::pull_n ],   adp_1_to_1_2 [adp::tsk::push_1], 1, thread_pinnig, { 4 });
	tools::Chain chain_stage3(  adp_1_to_1_2 [adp::tsk::pull_n ],   adp_1_to_1_3 [adp::tsk::push_1], 1, thread_pinnig, { 5 });
	tools::Chain chain_stage4(  adp_1_to_1_3 [adp::tsk::pull_n ],   adp_1_to_1_4 [adp::tsk::push_1], 1, thread_pinnig, { 6 });
	tools::Chain chain_stage5(  adp_1_to_1_4 [adp::tsk::pull_n ],   adp_1_to_n   [adp::tsk::push_1], 1, thread_pinnig, { 7 });
	tools::Chain chain_stage7(  adp_n_to_1   [adp::tsk::pull_1 ], (*sink        )[snk::tsk::send  ], 1, thread_pinnig, { 8 });

	std::vector<tools::Chain*> chain_stages = { &chain_stage0,          &chain_stage1, &chain_stage2,
	                                            &chain_stage3,          &chain_stage4, &chain_stage5,
	                                            &chain_stage6_parallel, &chain_stage7,                };

	for (size_t cs = 0; cs < chain_stages.size(); cs++)
	{
		std::ofstream fs("chain_stage" + std::to_string(cs) + ".dot");
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
	for (size_t s = 0; s < chain_stages.size(); s++)
	{
		const bool last_stage = s == chain_stages.size() -1;
		auto cs = chain_stages[s];
		threads.push_back(std::thread([cs, last_stage, &terminal_stats, &stats_file, &stop_threads]() {
			cs->exec([last_stage, &terminal_stats, &stats_file]()
			{
				if (last_stage) terminal_stats.temp_report(stats_file);
				return terminal_stats.is_interrupt();
			});
			stop_threads();
		}));
	}

	// std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	// stop_threads();

	// wait all the pipeline threads here
	for (auto &t : threads)
		t.join();
#else
	(*framer      )[frm::sck::remove_plh   ::Y_N1].bind((*sync_fine_pf)[sff::sck::synchronize  ::Y_N2]);
	(*estimator   )[est::sck::estimate     ::X_N ].bind((*framer      )[frm::sck::remove_plh   ::Y_N2]);
	(*modem       )[mdm::sck::demodulate_wg::H_N ].bind((*estimator   )[est::sck::estimate     ::H_N ]);
	(*modem       )[mdm::sck::demodulate_wg::Y_N1].bind((*framer      )[frm::sck::remove_plh   ::Y_N2]);
	(*itl_rx      )[itl::sck::deinterleave ::itl ].bind((*modem       )[mdm::sck::demodulate_wg::Y_N2]);
	(*LDPC_decoder)[dec::sck::decode_siho  ::Y_N ].bind((*itl_rx      )[itl::sck::deinterleave ::nat ]);
	(*BCH_decoder )[dec::sck::decode_hiho  ::Y_N ].bind((*LDPC_decoder)[dec::sck::decode_siho  ::V_K ]);
	(*bb_scrambler)[scr::sck::descramble   ::Y_N1].bind((*BCH_decoder )[dec::sck::decode_hiho  ::V_K ]);
	(*monitor     )[mnt::sck::check_errors ::U   ].bind((*source      )[src::sck::generate     ::U_K ]);
	(*monitor     )[mnt::sck::check_errors ::V   ].bind((*bb_scrambler)[scr::sck::descramble   ::Y_N2]);
	(*sink        )[snk::sck::send         ::V   ].bind((*bb_scrambler)[scr::sck::descramble   ::Y_N2]);

	tools::Chain chain_sequential3((*radio)[rad::tsk::receive], (*sink)[snk::tsk::send]);
	std::ofstream fs3("chain_sequential3.dot");
	chain_sequential3.export_dot(fs3);

	// start the transmission chain
	chain_sequential3.exec([&monitor, &terminal_stats, &stats_file](const std::vector<int>& statuses)
	{
		if (statuses.back() != status_t::SKIPPED)
			terminal_stats.temp_report(stats_file);
		return terminal_stats.is_interrupt();
	});

	// stop the radio thread
	for (auto &m : chain_sequential3.get_modules<tools::Interface_waiting>())
		m->cancel_waiting();
#endif /* MULTI_THREADED */

	// display the performance (BER and FER) in the terminal
	terminal.final_report();
	terminal_stats.final_report(stats_file);

	if (params.stats)
	{
		std::vector<const module::Module*> modules_stats(modules.size());
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
		std::cout << "#" << std::endl << "# Chain sequential (" << chain_sequential3.get_n_threads() << " thread(s)): "
		          << std::endl;
		tools::Stats::show(chain_sequential3.get_tasks_per_types(), ordered);
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

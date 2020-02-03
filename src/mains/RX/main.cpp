#include <aff3ct.hpp>

#include "Factory/DVBS2O/DVBS2O.hpp"

using namespace aff3ct;

namespace aff3ct { namespace tools {
using Monitor_BFER_reduction = Monitor_reduction<module::Monitor_BFER<>>;
} }

#define MULTI_THREADED

int main(int argc, char** argv)
{
#ifdef MULTI_THREADED
	aff3ct::tools::Thread_pinning::init();
	aff3ct::tools::Thread_pinning::set_logs(true);

	// aff3ct::tools::Thread_pinning::example1();
	// aff3ct::tools::Thread_pinning::example2();
	// aff3ct::tools::Thread_pinning::example3();
	// aff3ct::tools::Thread_pinning::example4();
	// aff3ct::tools::Thread_pinning::example5();
	// aff3ct::tools::Thread_pinning::example6();
#endif

	// get the parameter to configure the tools and modules
	auto params = factory::DVBS2O(argc, argv);

	std::map<std::string,tools::header_list> headers;
	std::vector<factory::Factory*> param_vec;
	param_vec.push_back(&params);
	tools::Header::print_parameters(param_vec, true, std::cout);

	std::vector<std::unique_ptr<tools::Reporter>> reporters;
	std::unique_ptr<tools::Terminal> terminal;
	tools::Sigma<> noise;
	std::unique_ptr<tools::Monitor_BFER_reduction> monitor_red;

	// the list of the allocated modules for the simulation
	std::vector<const module::Module*> modules;
	std::vector<const module::Module*> modules_each_snr;

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

#ifdef MULTI_THREADED
	const bool active_waiting = false;
	std::unique_ptr<module::Adaptor_1_to_n> adp_1_to_n  (new module::Adaptor_1_to_n(             2 * params.pl_frame_size, typeid(float), 1, active_waiting, params.n_frames));
	std::unique_ptr<module::Adaptor_n_to_1> adp_n_to_1  (new module::Adaptor_n_to_1(params.K_bch,                          typeid(int  ), 1, active_waiting, params.n_frames));
	std::unique_ptr<module::Adaptor_1_to_n> adp_1_to_1_0(new module::Adaptor_1_to_n(params.osf * 2 * params.pl_frame_size, typeid(float), 1, active_waiting, params.n_frames));
	std::unique_ptr<module::Adaptor_1_to_n> adp_1_to_1_1(new module::Adaptor_1_to_n(params.osf * 2 * params.pl_frame_size, typeid(float), 1, active_waiting, params.n_frames));
	std::unique_ptr<module::Adaptor_1_to_n> adp_1_to_1_2(new module::Adaptor_1_to_n({(size_t)params.osf * 2 * params.pl_frame_size, (size_t)params.osf * 2 * params.pl_frame_size}, {typeid(float), typeid(int32_t)}, 1, active_waiting, params.n_frames));
#endif /* MULTI_THREADED */

	// manage noise
	modem    ->set_noise(noise);
	estimator->set_noise(noise);
	auto mdm_ptr = modem.get();
	noise.record_callback_update([mdm_ptr](){ mdm_ptr->notify_noise_update(); });

	LDPC_decoder ->set_custom_name("LDPC Decoder");
	BCH_decoder  ->set_custom_name("BCH Decoder" );
	sync_lr      ->set_custom_name("L&R F Syn"   );
	sync_fine_pf ->set_custom_name("Fine P/F Syn");
	sync_timing  ->set_custom_name("Gardner Syn" );
	sync_frame   ->set_custom_name("Frame Syn"   );
	matched_flt  ->set_custom_name("Matched Flt" );
	sync_coarse_f->set_custom_name("Coarse_Synch");
	sync_step_mf ->set_custom_name("MF_Synch"    );
#ifdef MULTI_THREADED
	adp_1_to_n   ->set_custom_name("Adp_1_to_n"  );
	adp_n_to_1   ->set_custom_name("Adp_n_to_1"  );
	adp_1_to_1_0 ->set_custom_name("Adp_1_to_1_0");
	adp_1_to_1_1 ->set_custom_name("Adp_1_to_1_1");
	adp_1_to_1_2 ->set_custom_name("Adp_1_to_1_2");
#endif /* MULTI_THREADED */

	// fill the list of modules
	modules = { bb_scrambler.get(), BCH_decoder .get(), source       .get(), LDPC_decoder      ,
	            itl_rx      .get(), modem       .get(), framer       .get(), pl_scrambler.get(),
	            monitor     .get(), freq_shift  .get(), sync_lr      .get(), sync_fine_pf.get(),
	            radio       .get(), sync_frame  .get(), sync_coarse_f.get(), matched_flt .get(),
	            sync_timing .get(), sync_step_mf.get(), mult_agc     .get(), sink        .get(),
	            estimator   .get(), front_agc   .get(),
#ifdef MULTI_THREADED
	            adp_1_to_n  .get(), adp_n_to_1  .get(), adp_1_to_1_0 .get(), adp_1_to_1_1.get(),
	            adp_1_to_1_2.get(),
#endif /* MULTI_THREADED */
	          };

	// configuration of the module tasks
	for (auto& m : modules)
		for (auto& ta : m->tasks)
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

	int n_phase = 1;
	int m = 0;
	auto print_metrics = [&](bool last = false)
	{
		if (m == 0)
		{
			std::cout << "# LEARNING PHASE" << std::endl;
			std::cout << "# --------------" << std::endl;
			std::cout << "# -------|-------|-----------------|---------|-------------------|-------------------|-------------------" << std::endl;
			std::cout << "#  Phase |    m  |        mu       |  Frame  |      PLL CFO      |      LR CFO       |       F CFO       " << std::endl;
			std::cout << "# -------|-------|-----------------|---------|-------------------|-------------------|-------------------" << std::endl;
		}
		char buf[256];
		char pattern[] = "#    %2d  |  %4d |   %2.6e  |  %6d |    %+2.6e  |    %+2.6e  |    %+2.6e  ";
		sprintf(buf, pattern, n_phase, m,
				sync_timing ->get_mu(),
				sync_coarse_f->get_estimated_freq(),
				sync_coarse_f->get_curr_idx(),
				sync_lr      ->get_estimated_freq() / (float)params.osf,
				sync_fine_pf ->get_estimated_freq() / (float)params.osf);
		if (last)
			std::cout << buf << std::endl;
		else
		{
			std::cerr << buf << "\r";
			std::cerr.flush();
		}
	};

	using namespace module;

#ifdef MULTI_THREADED
	// parallel chain
	(*sync_fine_pf)[sff::sck::synchronize  ::X_N1].bind((*adp_1_to_n  )[adp::sck::pull_n       ::out1]);
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
	(*adp_n_to_1  )[adp::sck::push_n       ::in1 ].bind((*bb_scrambler)[scr::sck::descramble   ::Y_N2]);

	std::cout << "Cloning the modules of the parallel chain... ";
	std::cout.flush();
	tools::Chain chain_parallel((*adp_1_to_n)[module::adp::tsk::pull_n],
	                            (*adp_n_to_1)[module::adp::tsk::push_n],
	                            15);
	std::ofstream f("chain_parallel.dot");
	chain_parallel.export_dot(f);
	std::cout << "Done." << std::endl;
#endif /* MULTI_THREADED */

	// ================================================================================================================
	// LEARNING PHASE 1 & 2 ===========================================================================================

	(*front_agc   )[mlt::sck::imultiply  ::X_N  ].bind((*radio       )[rad::sck::receive    ::Y_N1 ]);
	(*sync_step_mf)[smf::sck::synchronize::X_N1 ].bind((*front_agc   )[mlt::sck::imultiply  ::Z_N  ]);
	(*sync_step_mf)[smf::sck::synchronize::delay].bind((*sync_frame  )[sfm::sck::synchronize::delay]);
	(*mult_agc    )[mlt::sck::imultiply  ::X_N  ].bind((*sync_step_mf)[smf::sck::synchronize::Y_N2 ]);
	(*sync_frame  )[sfm::sck::synchronize::X_N1 ].bind((*mult_agc    )[mlt::sck::imultiply  ::Z_N  ]);
	(*pl_scrambler)[scr::sck::descramble ::Y_N1 ].bind((*sync_frame  )[sfm::sck::synchronize::Y_N2 ]);

	tools::Chain chain_sequential1((*radio)[rad::tsk::receive], (*pl_scrambler)[scr::tsk::descramble]);
	std::ofstream fs1("chain_sequential1.dot");
	chain_sequential1.export_dot(fs1);

	int limit = 150;
	sync_coarse_f->set_PLL_coeffs(1, 1/std::sqrt(2.0), 1e-4);
	chain_sequential1.exec([&]()
	{
		print_metrics((limit == 150 && m >= 150) || m >= 300);
		if (limit == 150 && m >= 150)
		{
			n_phase = 2;
			limit = 300;
			sync_coarse_f->set_PLL_coeffs(1, 1/std::sqrt(2.0), 5e-5);
		}
		const auto stop = m >= limit;
		m += params.n_frames;
		return stop;
	});

	// ================================================================================================================
	// LEARNING PHASE 3 ===============================================================================================

	(*radio       )[rad::sck::receive    ::Y_N1 ].reset();
	(*front_agc   )[mlt::sck::imultiply  ::X_N  ].reset();
	(*front_agc   )[mlt::sck::imultiply  ::Z_N  ].reset();
	(*sync_frame  )[sfm::sck::synchronize::delay].reset();
	(*mult_agc    )[mlt::sck::imultiply  ::X_N  ].reset();
	(*mult_agc    )[mlt::sck::imultiply  ::Z_N  ].reset();
	(*sync_frame  )[sfm::sck::synchronize::X_N1 ].reset();
	(*sync_frame  )[sfm::sck::synchronize::Y_N2 ].reset();
	(*pl_scrambler)[scr::sck::descramble ::Y_N1 ].reset();

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

	tools::Chain chain_sequential2((*radio)[rad::tsk::receive], (*sync_lr)[sff::tsk::synchronize]);
	std::ofstream fs2("chain_sequential2.dot");
	chain_sequential2.export_dot(fs2);

	n_phase = 3;
	chain_sequential2.exec([&]()
	{
		const auto stop = m >= 500;
		print_metrics(stop);
		m += params.n_frames;
		return stop;
	});

	for (auto& m : modules)
		for (auto& ta : m->tasks)
			ta->reset();

#ifdef MULTI_THREADED
	// allocate a common monitor module to reduce all the monitors
	monitor_red = std::unique_ptr<tools::Monitor_BFER_reduction>(new tools::Monitor_BFER_reduction(
		chain_parallel.get_modules<module::Monitor_BFER<>>()));
#else
	(*sync_fine_pf)[sff::sck::synchronize  ::X_N1].bind((*sync_lr     )[sff::sck::synchronize  ::Y_N2]);
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

	monitor_red = std::unique_ptr<tools::Monitor_BFER_reduction>(new tools::Monitor_BFER_reduction(
		chain_sequential3.get_modules<module::Monitor_BFER<>>()));
#endif /* MULTI_THREADED */

	monitor_red->set_reduce_frequency(std::chrono::milliseconds(500));

	// allocate reporters to display results in the terminal
	reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_noise     <>( noise      ))); // report the noise values (Es/N0 and Eb/N0)
	reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_BFER      <>(*monitor_red))); // report the bit/frame error rates
	reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_throughput<>(*monitor_red))); // report the simulation throughputs

	// allocate a terminal that will display the collected data from the reporters
	terminal = std::unique_ptr<tools::Terminal>(new tools::Terminal_std(reporters));

	if (params.ter_freq != std::chrono::nanoseconds(0))
		terminal->start_temp_report(params.ter_freq);

	std::cout << "#"                    << std::endl;
	std::cout << "# TRANSMISSION PHASE" << std::endl;
	std::cout << "# ------------------" << std::endl;

	// display the legend in the terminal
	terminal->legend();

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

	(*adp_1_to_1_0 )[adp::sck::push_1     ::in1 ].bind((*radio        )[rad::sck::receive    ::Y_N1]);
	(*front_agc    )[mlt::sck::imultiply  ::X_N ].bind((*adp_1_to_1_0 )[adp::sck::pull_n     ::out1]);
	(*sync_coarse_f)[sfc::sck::synchronize::X_N1].bind((*front_agc    )[mlt::sck::imultiply  ::Z_N ]);
	(*matched_flt  )[flt::sck::filter     ::X_N1].bind((*sync_coarse_f)[sfc::sck::synchronize::Y_N2]);
	(*adp_1_to_1_1 )[adp::sck::push_1     ::in1 ].bind((*matched_flt  )[flt::sck::filter     ::Y_N2]);
	(*sync_timing  )[stm::sck::synchronize::X_N1].bind((*adp_1_to_1_1 )[adp::sck::pull_n     ::out1]);
	(*adp_1_to_1_2 )[adp::sck::push_1     ::in1 ].bind((*sync_timing  )[stm::sck::synchronize::Y_N1]);
	(*adp_1_to_1_2 )[adp::sck::push_1     ::in2 ].bind((*sync_timing  )[stm::sck::synchronize::B_N1]);
	(*sync_timing  )[stm::sck::extract    ::B_N1].bind((*adp_1_to_1_2 )[adp::sck::pull_n     ::out2]);
	(*sync_timing  )[stm::sck::extract    ::Y_N1].bind((*adp_1_to_1_2 )[adp::sck::pull_n     ::out1]);
	(*mult_agc     )[mlt::sck::imultiply  ::X_N ].bind((*sync_timing  )[stm::sck::extract    ::Y_N2]);
	(*sync_frame   )[sfm::sck::synchronize::X_N1].bind((*mult_agc     )[mlt::sck::imultiply  ::Z_N ]);
	(*pl_scrambler )[scr::sck::descramble ::Y_N1].bind((*sync_frame   )[sfm::sck::synchronize::Y_N2]);
	(*sync_lr      )[sff::sck::synchronize::X_N1].bind((*pl_scrambler )[scr::sck::descramble ::Y_N2]);
	(*adp_1_to_n   )[adp::sck::push_1     ::in1 ].bind((*sync_lr      )[sff::sck::synchronize::Y_N2]);
	// parallel chain
	(*sink         )[snk::sck::send       ::V   ].bind((*adp_n_to_1   )[adp::sck::pull_1     ::out1]);

	// create a chain per pipeline stage
	tools::Chain chain_stage0((*radio       )[rad::tsk::receive], (*adp_1_to_1_0)[adp::tsk::push_1]);
	tools::Chain chain_stage1((*adp_1_to_1_0)[adp::tsk::pull_n ], (*adp_1_to_1_1)[adp::tsk::push_1]);
	tools::Chain chain_stage2((*adp_1_to_1_1)[adp::tsk::pull_n ], (*adp_1_to_1_2)[adp::tsk::push_1]);
	tools::Chain chain_stage3((*adp_1_to_1_2)[adp::tsk::pull_n ], (*adp_1_to_n  )[adp::tsk::push_1]);
	tools::Chain chain_stage4((*adp_n_to_1  )[adp::tsk::pull_1 ], (*sink        )[snk::tsk::send  ]);

	std::vector<tools::Chain*> chain_stages = { &chain_stage0, &chain_stage1, &chain_stage2,
	                                            &chain_stage3, &chain_stage4                 };

	for (size_t cs = 0; cs < chain_stages.size(); cs++)
	{
		std::ofstream fs("chain_stage" + std::to_string(cs) + ".dot");
		chain_stages[cs]->export_dot(fs);
	}

	// reset the stats of all the tasks
	for (auto &cs : chain_stages)
		for (auto &tt : cs->get_tasks_per_threads())
			for (auto &t : tt) t->reset();

	// enable thread pinning
	chain_parallel.set_thread_pinning(true, { 24, 25, 26, 27, 28,
	                                          12, 29, 13, 30, 14,
	                                          31, 15, 32, 16, 33,
	                                          17, 34, 18, 35, 19,
	                                          36, 20, 37, 21, 38,
	                                          22, 39, 23, 39, 40,
	                                          41, 42, 43, 44, 45,
	                                          46, 47              });
	size_t puid = 1;
	for (auto &cs : chain_stages)
		cs->set_thread_pinning(true, { puid++ });

	// function to wake up and stop all the threads
	auto stop_threads = [&chain_parallel, &chain_stages]()
	{
		for (auto &m : chain_parallel.get_modules<tools::Interface_waiting>())
			m->cancel_waiting();
		for (auto &cs : chain_stages)
			for (auto &m : cs->get_modules<tools::Interface_waiting>())
				m->cancel_waiting();
	};

	// start the pipeline threads
	std::vector<std::thread> threads;
	size_t tid = 0;
	for (auto &cs : chain_stages)
	{
		threads.push_back(std::thread([cs, tid, &terminal, &stop_threads]() {
			cs->exec([&terminal]() { return terminal->is_interrupt(); } );
			stop_threads();
		}));
		tid++;
	}

	// start the parallel chain
	chain_parallel.exec([&monitor_red, &terminal]()
	{
		monitor_red->is_done();
		return terminal->is_interrupt();
	});
	stop_threads();

	// wait all the pipeline threads here
	for (auto &t : threads)
		t.join();
#else
	// start the transmission chain
	chain_sequential3.exec([&monitor_red, &terminal]()
	{
		monitor_red->is_done();
		return terminal->is_interrupt();
	});

	// stop the radio thread
	for (auto &m : chain_sequential3.get_modules<tools::Interface_waiting>())
		m->cancel_waiting();
#endif /* MULTI_THREADED */

	// final reduction
	monitor_red->reduce();

	// display the performance (BER and FER) in the terminal
	terminal->final_report();

	if (params.stats)
	{
		std::vector<const module::Module*> modules_stats(modules.size());
		for (size_t m = 0; m < modules.size(); m++)
			modules_stats.push_back(modules[m]);

		const auto ordered = true;
		// std::cout << "#" << std::endl;
		// tools::Stats::show(modules_stats, ordered);
#ifdef MULTI_THREADED
		for (size_t cs = 0; cs < chain_stages.size(); cs++)
		{
			std::cout << "#" << std::endl << "# Chain stage " << cs << " (" << chain_stages[cs]->get_n_threads()
			                 << " thread(s)): " << std::endl;
			tools::Stats::show(chain_stages[cs]->get_tasks_per_types(), ordered);
		}
		std::cout << "#" << std::endl << "# Chain parallel (" << chain_parallel.get_n_threads() << " thread(s)): "
		          << std::endl;
		tools::Stats::show(chain_parallel.get_tasks_per_types(), ordered);
#else
		std::cout << "#" << std::endl << "# Chain sequential (" << chain_sequential3.get_n_threads() << " thread(s)): "
		          << std::endl;
		tools::Stats::show(chain_sequential3.get_tasks_per_types(), ordered);
#endif /* MULTI_THREADED */
	}

	std::cout << "#" << std::endl;
	std::cout << "# End of the simulation" << std::endl;

#ifdef MULTI_THREADED
	aff3ct::tools::Thread_pinning::destroy();
#endif

	return EXIT_SUCCESS;
}

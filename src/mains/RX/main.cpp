#include <aff3ct.hpp>

#include "Factory/DVBS2O/DVBS2O.hpp"

using namespace aff3ct;

namespace aff3ct { namespace tools {
using Monitor_BFER_reduction = Monitor_reduction<module::Monitor_BFER<>>;
} }

int main(int argc, char** argv)
{
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

	std::unique_ptr<module::Adaptor_1_to_n> adp_1_to_n  (new module::Adaptor_1_to_n(             2 * params.pl_frame_size, typeid(float), 16, false, params.n_frames));
	std::unique_ptr<module::Adaptor_n_to_1> adp_n_to_1  (new module::Adaptor_n_to_1(params.K_bch,                          typeid(int  ), 16, false, params.n_frames));
	std::unique_ptr<module::Adaptor_1_to_n> adp_1_to_1_1(new module::Adaptor_1_to_n(params.osf * 2 * params.pl_frame_size, typeid(float), 16, false, params.n_frames));
	std::unique_ptr<module::Adaptor_1_to_n> adp_1_to_1_2(new module::Adaptor_1_to_n(             2 * params.pl_frame_size, typeid(float), 16, false, params.n_frames));
	std::unique_ptr<module::Adaptor_1_to_n> adp_1_to_1_3(new module::Adaptor_1_to_n(             2 * params.pl_frame_size, typeid(float), 16, false, params.n_frames));
	std::unique_ptr<module::Adaptor_1_to_n> adp_1_to_1_4(new module::Adaptor_1_to_n(             2 * params.pl_frame_size, typeid(float), 16, false, params.n_frames));
	std::vector<module::Adaptor*> adaptors_1_to_1 = { adp_1_to_1_1.get(), adp_1_to_1_2.get(), adp_1_to_1_3.get(), adp_1_to_1_4.get() };

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
	adp_1_to_n   ->set_custom_name("Adp_1_to_n"  );
	adp_n_to_1   ->set_custom_name("Adp_n_to_1"  );
	adp_1_to_1_1 ->set_custom_name("Adp_1_to_1_1");
	adp_1_to_1_2 ->set_custom_name("Adp_1_to_1_2");
	adp_1_to_1_3 ->set_custom_name("Adp_1_to_1_3");
	adp_1_to_1_4 ->set_custom_name("Adp_1_to_1_4");

	// fill the list of modules
	modules = { bb_scrambler.get(), BCH_decoder .get(), source       .get(), LDPC_decoder      ,
	            itl_rx      .get(), modem       .get(), framer       .get(), pl_scrambler.get(),
	            monitor     .get(), freq_shift  .get(), sync_lr      .get(), sync_fine_pf.get(),
	            radio       .get(), sync_frame  .get(), sync_coarse_f.get(), matched_flt .get(),
	            sync_timing .get(), sync_step_mf.get(), mult_agc     .get(), sink        .get(),
	            estimator   .get(), front_agc   .get(), adp_1_to_n   .get(), adp_n_to_1  .get(),
	            adp_1_to_1_1.get(), adp_1_to_1_2.get(), adp_1_to_1_3 .get(), adp_1_to_1_4.get() };

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

	using namespace module;

	(*sync_lr       )[sff::sck::synchronize  ::X_N1 ].bind((*pl_scrambler  )[scr::sck::descramble   ::Y_N2 ]);
	(*adp_1_to_n    )[adp::sck::put_1        ::in   ].bind((*sync_lr       )[sff::sck::synchronize  ::Y_N2 ]);
	(*sync_fine_pf  )[sff::sck::synchronize  ::X_N1 ].bind((*adp_1_to_n    )[adp::sck::pull_n       ::out  ]);
	(*framer        )[frm::sck::remove_plh   ::Y_N1 ].bind((*sync_fine_pf  )[sff::sck::synchronize  ::Y_N2 ]);
	(*estimator     )[est::sck::estimate     ::X_N  ].bind((*framer        )[frm::sck::remove_plh   ::Y_N2 ]);
	(*modem         )[mdm::sck::demodulate_wg::H_N  ].bind((*estimator     )[est::sck::estimate     ::H_N  ]);
	(*modem         )[mdm::sck::demodulate_wg::Y_N1 ].bind((*framer        )[frm::sck::remove_plh   ::Y_N2 ]);
	(*itl_rx        )[itl::sck::deinterleave ::itl  ].bind((*modem         )[mdm::sck::demodulate_wg::Y_N2 ]);
	(*LDPC_decoder  )[dec::sck::decode_siho  ::Y_N  ].bind((*itl_rx        )[itl::sck::deinterleave ::nat  ]);
	(*BCH_decoder   )[dec::sck::decode_hiho  ::Y_N  ].bind((*LDPC_decoder  )[dec::sck::decode_siho  ::V_K  ]);
	(*bb_scrambler  )[scr::sck::descramble   ::Y_N1 ].bind((*BCH_decoder   )[dec::sck::decode_hiho  ::V_K  ]);
	(*front_agc     )[mlt::sck::imultiply    ::X_N  ].bind((*radio         )[rad::sck::receive      ::Y_N1 ]);
	(*sync_step_mf  )[smf::sck::synchronize  ::X_N1 ].bind((*front_agc     )[mlt::sck::imultiply    ::Z_N  ]);
	(*sync_step_mf  )[smf::sck::synchronize  ::delay].bind((*sync_frame    )[sfm::sck::synchronize  ::delay]);
	(*mult_agc      )[mlt::sck::imultiply    ::X_N  ].bind((*sync_step_mf  )[smf::sck::synchronize  ::Y_N2 ]);
	(*adp_1_to_1_3  )[adp::sck::put_1        ::in   ].bind((*mult_agc      )[mlt::sck::imultiply    ::Z_N  ]);
	(*sync_frame    )[sfm::sck::synchronize  ::X_N1 ].bind((*adp_1_to_1_3  )[adp::sck::pull_n       ::out  ]);
	(*adp_1_to_1_4  )[adp::sck::put_1        ::in   ].bind((*sync_frame    )[sfm::sck::synchronize  ::Y_N2 ]);
	(*pl_scrambler  )[scr::sck::descramble   ::Y_N1 ].bind((*adp_1_to_1_4  )[adp::sck::pull_n       ::out  ]);
	(*monitor       )[mnt::sck::check_errors ::U    ].bind((*source        )[src::sck::generate     ::U_K  ]);
	(*monitor       )[mnt::sck::check_errors ::V    ].bind((*bb_scrambler  )[scr::sck::descramble   ::Y_N2 ]);
	(*adp_n_to_1    )[adp::sck::put_n        ::in   ].bind((*bb_scrambler  )[scr::sck::descramble   ::Y_N2 ]);
	(*sink          )[snk::sck::send         ::V    ].bind((*adp_n_to_1    )[adp::sck::pull_1       ::out  ]);

	tools::Chain chain_parallel((*adp_1_to_n)[module::adp::tsk::pull_n],
	                            (*adp_n_to_1)[module::adp::tsk::put_n ],
	                            12);
	// DEBUG
	std::ofstream f("chain_parallel.dot");
	chain_parallel.export_dot(f);

	freq_shift   ->reset();
	sync_coarse_f->reset();
	sync_coarse_f->set_PLL_coeffs(1, 1/std::sqrt(2.0), 1e-4);
	matched_flt  ->reset();
	sync_timing  ->reset();
	sync_frame   ->reset();
	sync_lr      ->reset();
	sync_fine_pf ->reset();

	std::cout << "# LEARNING PHASE" << std::endl;
	std::cout << "# --------------" << std::endl;
	char buf[256];
	char head_lines[]  = "# -------|-------|-----------------|---------|-------------------|-------------------|-------------------";
	char heads[]  =      "#  Phase |    m  |        mu       |  Frame  |      PLL CFO      |      LR CFO       |       F CFO       ";
	char pattern[]  =    "#    %2d  |  %4d |   %2.6e  |  %6d |    %+2.6e  |    %+2.6e  |    %+2.6e  ";
	std::cerr <<head_lines <<"\n" << heads <<"\n" <<head_lines <<"\n";
	std::cerr.flush();

	int n_phase = 1;
	for (int m = 0; m < 500; m += params.n_frames)
	{
		if (n_phase < 3)
		{
			(*radio       )[rad::tsk::receive    ].exec();
			(*front_agc   )[mlt::tsk::imultiply  ].exec();
			(*sync_step_mf)[smf::tsk::synchronize].exec();
			(*mult_agc    )[mlt::tsk::imultiply  ].exec();
			(*adp_1_to_1_3)[adp::tsk::put_1      ].exec();
			(*adp_1_to_1_3)[adp::tsk::pull_n     ].exec();
			(*sync_frame  )[sfm::tsk::synchronize].exec();
			(*adp_1_to_1_4)[adp::tsk::put_1      ].exec();
			(*adp_1_to_1_4)[adp::tsk::pull_n     ].exec();
			(*pl_scrambler)[scr::tsk::descramble ].exec();
		}
		else // n_phase == 3
		{
			try
			{
				(*radio        )[rad::tsk::receive    ].exec();
				(*front_agc    )[mlt::tsk::imultiply  ].exec();
				(*sync_coarse_f)[sfc::tsk::synchronize].exec();
				(*matched_flt  )[flt::tsk::filter     ].exec();
				(*adp_1_to_1_1 )[adp::tsk::put_1      ].exec();
				(*adp_1_to_1_1 )[adp::tsk::pull_n     ].exec();
				(*sync_timing  )[stm::tsk::synchronize].exec(); // can raise the 'tools::processing_aborted' exception
				(*adp_1_to_1_2 )[adp::tsk::put_1      ].exec();
				(*adp_1_to_1_2 )[adp::tsk::pull_n     ].exec();
				(*mult_agc     )[mlt::tsk::imultiply  ].exec();
				(*adp_1_to_1_3 )[adp::tsk::put_1      ].exec();
				(*adp_1_to_1_3 )[adp::tsk::pull_n     ].exec();
				(*sync_frame   )[sfm::tsk::synchronize].exec();
				(*adp_1_to_1_4 )[adp::tsk::put_1      ].exec();
				(*adp_1_to_1_4 )[adp::tsk::pull_n     ].exec();
				(*pl_scrambler )[scr::tsk::descramble ].exec();
				(*sync_lr      )[sff::tsk::synchronize].exec();
				(*sync_fine_pf )[sff::tsk::synchronize].exec();
			}
			catch (tools::processing_aborted const&) {}
		}

		sprintf(buf, pattern, n_phase, m+1,
				sync_timing ->get_mu(),
				sync_coarse_f->get_estimated_freq(),
				sync_coarse_f->get_curr_idx(),
				sync_lr      ->get_estimated_freq() / (float)params.osf,
				sync_fine_pf ->get_estimated_freq() / (float)params.osf);
		std::cerr << buf << "\r";
		std::cerr.flush();

		if (m > 149 && n_phase == 1)
		{
			m = 150;
			n_phase++;
			std::cerr << buf << std::endl;
			sync_coarse_f->set_PLL_coeffs(1, 1/std::sqrt(2.0), 5e-5);
		}

		if (m > 299 && n_phase == 2)
		{
			m = 300;
			n_phase++;
			std::cerr << buf << std::endl;
			(*sync_coarse_f )[sfc::sck::synchronize::X_N1].bind((*front_agc     )[mlt::sck::imultiply  ::Z_N ]);
			(*matched_flt   )[flt::sck::filter     ::X_N1].bind((*sync_coarse_f )[sfc::sck::synchronize::Y_N2]);
			(*adp_1_to_1_1  )[adp::sck::put_1      ::in  ].bind((*matched_flt   )[flt::sck::filter     ::Y_N2]);
			(*sync_timing   )[stm::sck::synchronize::X_N1].bind((*adp_1_to_1_1  )[adp::sck::pull_n     ::out ]);
			(*adp_1_to_1_2  )[adp::sck::put_1      ::in  ].bind((*sync_timing   )[stm::sck::synchronize::Y_N2]);
			(*mult_agc      )[mlt::sck::imultiply  ::X_N ].bind((*adp_1_to_1_2  )[adp::sck::pull_n     ::out ]);
			(*sync_frame    )[sfm::sck::synchronize::X_N1].bind((*mult_agc      )[mlt::sck::imultiply  ::Z_N ]);
		}
	}
	std::cerr << buf << "\n" << head_lines << "\n";

	for (auto& m : modules)
		for (auto& ta : m->tasks)
			ta->reset();

	// allocate a common monitor module to reduce all the monitors
	monitor_red = std::unique_ptr<tools::Monitor_BFER_reduction>(new tools::Monitor_BFER_reduction(
		chain_parallel.get_modules<module::Monitor_BFER<>>()));
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

	auto stop_threads = [&chain_parallel, &adaptors_1_to_1]()
	{
		for (auto &m : chain_parallel.get_modules<tools::Interface_waiting>())
			m->cancel_waiting();
		for (auto adp : adaptors_1_to_1)
			adp->cancel_waiting();
	};

	std::vector<std::thread> threads;
	auto exec_pipeline_thread = [&](std::function<void()> sequence)
	{
		try
		{
			while (!terminal->is_interrupt())
			{
				try
				{
					sequence();
				}
				catch (tools::processing_aborted const&) {}
			}
		}
		catch (tools::waiting_canceled const&) {}
		stop_threads();
	};

	threads.push_back(std::thread([&]() { exec_pipeline_thread([&]() {
		(*source       )[src::tsk::generate   ].exec(); // sequential |   7 us
		(*radio        )[rad::tsk::receive    ].exec(); // sequential |   ? us
		(*front_agc    )[mlt::tsk::imultiply  ].exec(); // parallel   |  27 us
		(*sync_coarse_f)[sfc::tsk::synchronize].exec(); // sequential | 313 us
		(*matched_flt  )[flt::tsk::filter     ].exec(); // sequential | 160 us
		(*adp_1_to_1_1 )[adp::tsk::put_1      ].exec(); // sequential |  10 us
	}); }));
	threads.push_back(std::thread([&]() { exec_pipeline_thread([&]() {
		(*adp_1_to_1_1 )[adp::tsk::pull_n     ].exec(); // sequential |  10 us |
		(*sync_timing  )[stm::tsk::synchronize].exec(); // sequential | 548 us | can raise an exception
		(*adp_1_to_1_2 )[adp::tsk::put_1      ].exec(); // sequential |  20 us |
	}); }));
	threads.push_back(std::thread([&]() { exec_pipeline_thread([&]() {
		(*adp_1_to_1_2 )[adp::tsk::pull_n     ].exec(); // sequential |  20 us
		(*mult_agc     )[mlt::tsk::imultiply  ].exec(); // parallel   |  27 us
		(*adp_1_to_1_3 )[adp::tsk::put_1      ].exec(); // sequential |  10 us
	}); }));
	threads.push_back(std::thread([&]() { exec_pipeline_thread([&]() {
		(*adp_1_to_1_3 )[adp::tsk::pull_n     ].exec(); // sequential |  10 us
		(*sync_frame   )[sfm::tsk::synchronize].exec(); // sequential | 518 us
		(*adp_1_to_1_4 )[adp::tsk::put_1      ].exec(); // sequential |  10 us
	}); }));
	threads.push_back(std::thread([&]() { exec_pipeline_thread([&]() {
		(*adp_1_to_1_4 )[adp::tsk::pull_n     ].exec(); // sequential |  10 us
		(*pl_scrambler )[scr::tsk::descramble ].exec(); // parallel   |   5 us
		(*sync_lr      )[sff::tsk::synchronize].exec(); // sequential | 186 us
		(*adp_1_to_n   )[adp::tsk::put_1      ].exec(); // sequential |  16 us
	}); }));
	threads.push_back(std::thread([&]() { exec_pipeline_thread([&]() {
		(*adp_n_to_1   )[adp::tsk::pull_1     ].exec(); // sequential |   5 us
		(*sink         )[snk::tsk::send       ].exec(); // sequential |  40 us
	}); }));

	chain_parallel.exec([&monitor_red, &terminal]()
	{
		monitor_red->is_done();
		return terminal->is_interrupt();
	});
	stop_threads();

	for (auto &t : threads)
		t.join();

	// final reduction
	monitor_red->reduce();

	// display the performance (BER and FER) in the terminal
	terminal->final_report();

	if (params.stats)
	{
		std::vector<const module::Module*> modules_stats(modules.size());
		for (size_t m = 0; m < modules.size(); m++)
			modules_stats.push_back(modules[m]);

		std::cout << "#" << std::endl;
		const auto ordered = true;
		tools::Stats::show(modules_stats, ordered);
	}

	std::cout << "#" << std::endl;
	std::cout << "# End of the simulation" << std::endl;

	return EXIT_SUCCESS;
}

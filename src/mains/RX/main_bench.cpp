#include <chrono>
#include <fstream>
#include <aff3ct.hpp>

#include "Factory/DVBS2/DVBS2.hpp"
#ifdef DVBS2_LINK_UHD
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
#endif /* MULTI_THREADED */

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
#endif /* MULTI_THREADED */

	// setup signal handlers
	tools::setup_signal_handler();

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
	uptr<Sink<>                     > sink         (factory::DVBS2::build_sink                    <>(params            ));
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

	// the full transmission chain binding
	(*front_agc    )[mlt::sck::imultiply   ::X_N    ] = (*radio        )[rad::sck::receive     ::Y_N1];
	(*sync_coarse_f)[sfc::sck::synchronize ::X_N1   ] = (*front_agc    )[mlt::sck::imultiply   ::Z_N ];
	(*matched_flt  )[flt::sck::filter      ::X_N1   ] = (*sync_coarse_f)[sfc::sck::synchronize ::Y_N2];
	(*sync_timing  )[stm::sck::synchronize ::X_N1   ] = (*matched_flt  )[flt::sck::filter      ::Y_N2];
	(*sync_timing  )[stm::sck::extract     ::B_N1   ] = (*sync_timing  )[stm::sck::synchronize ::B_N1];
	(*sync_timing  )[stm::sck::extract     ::Y_N1   ] = (*sync_timing  )[stm::sck::synchronize ::Y_N1];
	(*mult_agc     )[mlt::sck::imultiply   ::X_N    ] = (*sync_timing  )[stm::sck::extract     ::Y_N2];
	(*sync_frame   )[sfm::sck::synchronize ::X_N1   ] = (*mult_agc     )[mlt::sck::imultiply   ::Z_N ];
	(*pl_scrambler )[scr::sck::descramble  ::Y_N1   ] = (*sync_frame   )[sfm::sck::synchronize ::Y_N2];
	(*sync_fine_lr )[sff::sck::synchronize ::X_N1   ] = (*pl_scrambler )[scr::sck::descramble  ::Y_N2];
	(*sync_fine_pf )[sff::sck::synchronize ::X_N1   ] = (*sync_fine_lr )[sff::sck::synchronize ::Y_N2];
	(*framer       )[frm::sck::remove_plh  ::Y_N1   ] = (*sync_fine_pf )[sff::sck::synchronize ::Y_N2];
	(*estimator    )[est::sck::estimate    ::X_N    ] = (*framer       )[frm::sck::remove_plh  ::Y_N2];
	(*modem        )[mdm::sck::demodulate  ::CP     ] = (*estimator    )[est::sck::estimate    ::SIG ];
	(*modem        )[mdm::sck::demodulate  ::Y_N1   ] = (*framer       )[frm::sck::remove_plh  ::Y_N2];
	(*itl_rx       )[itl::sck::deinterleave::itl    ] = (*modem        )[mdm::sck::demodulate  ::Y_N2];
	(*LDPC_decoder )[dec::sck::decode_siho ::Y_N    ] = (*itl_rx       )[itl::sck::deinterleave::nat ];
	(*BCH_decoder  )[dec::sck::decode_hiho ::Y_N    ] = (*LDPC_decoder )[dec::sck::decode_siho ::V_K ];
	(*bb_scrambler )[scr::sck::descramble  ::Y_N1   ] = (*BCH_decoder  )[dec::sck::decode_hiho ::V_K ];
	(*sink         )[snk::sck::send        ::in_data] = (*bb_scrambler )[scr::sck::descramble  ::Y_N2];

	// first stages of the whole transmission sequence
	const std::vector<runtime::Task*> firsts_t = { &(*radio)[rad::tsk::receive] };

#ifdef MULTI_THREADED
	auto start_clone = std::chrono::system_clock::now();
	std::cout << "Cloning the modules of the parallel sequence... ";
	std::cout.flush();

	// pipeline definition with separation stages
	const std::vector<std::tuple<std::vector<runtime::Task*>,
	                             std::vector<runtime::Task*>,
	                             std::vector<runtime::Task*>>> sep_stages =
	{ // pipeline stage 0
	  std::make_tuple<std::vector<runtime::Task*>, std::vector<runtime::Task*>, std::vector<runtime::Task*>>(
	    { &(*radio)[rad::tsk::receive], },
	    { &(*radio)[rad::tsk::receive], },
	    { /* no exclusions in this stage */ } ),
	  // pipeline stage 1
	  std::make_tuple<std::vector<runtime::Task*>, std::vector<runtime::Task*>, std::vector<runtime::Task*>>(
	    { &(*front_agc)[mlt::tsk::imultiply] },
	    { &(*matched_flt)[flt::tsk::filter] },
	    { /* no exclusions in this stage */ } ),
	  // pipeline stage 2
	  std::make_tuple<std::vector<runtime::Task*>, std::vector<runtime::Task*>, std::vector<runtime::Task*>>(
	    { &(*sync_timing)[stm::tsk::synchronize] },
	    { &(*sync_timing)[stm::tsk::synchronize] },
	    { /* no exclusions in this stage */ } ),
	  // pipeline stage 3
	  std::make_tuple<std::vector<runtime::Task*>, std::vector<runtime::Task*>, std::vector<runtime::Task*>>(
	    { &(*sync_timing)[stm::tsk::extract] },
	    { &(*sync_frame)[sfm::tsk::synchronize] },
	    { /* no exclusions in this stage */ } ),
	  // pipeline stage 4
	  std::make_tuple<std::vector<runtime::Task*>, std::vector<runtime::Task*>, std::vector<runtime::Task*>>(
	    { &(*pl_scrambler)[scr::tsk::descramble]  },
	    { &(*sync_fine_pf)[sff::tsk::synchronize] },
	    { /* no exclusions in this stage */ } ),
	  // pipeline stage 5
	  std::make_tuple<std::vector<runtime::Task*>, std::vector<runtime::Task*>, std::vector<runtime::Task*>>(
	    { &(*framer)[frm::tsk::remove_plh] },
	    { &(*estimator)[est::tsk::estimate] },
	    { &(*modem)[mdm::tsk::demodulate] } ),
	  // pipeline stage 6
	  std::make_tuple<std::vector<runtime::Task*>, std::vector<runtime::Task*>, std::vector<runtime::Task*>>(
	    { &(*modem)[mdm::tsk::demodulate] },
	    { &(*bb_scrambler)[scr::tsk::descramble] },
	    { } ),
	  // pipeline stage 7
	  std::make_tuple<std::vector<runtime::Task*>, std::vector<runtime::Task*>, std::vector<runtime::Task*>>(
	    { &(*sink)[snk::tsk::send] },
	    { /* end of the sequence */ },
	    { /* no exclusions in this stage */ } ),
	};
	// number of threads per stages
	const std::vector<size_t> n_threads_per_stages = { 1, 1, 1, 1, 1, 1, 28, 1 };
	// synchronization buffer size between stages
	const std::vector<size_t> buffer_sizes(sep_stages.size() -1, 1);
	// type of waiting between stages (true = active, false = passive)
	const std::vector<bool> active_waitings(sep_stages.size() -1, active_waiting);
	// enable thread pinning
	const std::vector<bool> thread_pinnigs(sep_stages.size(), thread_pinnig);
	// process unit (pu) ids per stage for thread pinning
	const std::vector<std::vector<size_t>> puids = { { 2 },                            // for stage 0
	                                                 { 3 },                            // for stage 1
	                                                 { 4 },                            // for stage 2
	                                                 { 5 },                            // for stage 3
	                                                 { 6 },                            // for stage 4
	                                                 { 7 },                            // for stage 5
	                                                 { 12, 24, 25, 26, 27, 28, 29, 13, // for stage 6
	                                                   30, 14, 31, 15, 32, 16, 33, 17,
	                                                   34, 18, 35, 19, 36, 20, 37, 21,
	                                                   38, 22, 39, 23, 39, 40, 41, 42,
	                                                   43, 44, 45, 46, 47 },
	                                                 { 8 } };                          // for stage 7

	runtime::Pipeline pipeline_transmission(firsts_t, sep_stages, n_threads_per_stages, buffer_sizes, active_waitings,
	                                        thread_pinnigs, puids);

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
		tools::Thread_pinning::pin(0);
#else
	runtime::Sequence sequence_transmission(firsts_t);
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

	if (!params.no_wl_phases) {
		// ============================================================================================================
		// WAITING PHASE ==============================================================================================
		// ============================================================================================================
		auto start_waiting = std::chrono::system_clock::now();
		std::cout << "Waiting phase... ";
		std::cout.flush();

		// partial unbinding
		(*sync_coarse_f)[sfc::sck::synchronize::X_N1].unbind((*front_agc    )[mlt::sck::imultiply  ::Z_N ]);
		(*sync_timing  )[stm::sck::extract    ::B_N1].unbind((*sync_timing  )[stm::sck::synchronize::B_N1]);
		(*sync_timing  )[stm::sck::extract    ::Y_N1].unbind((*sync_timing  )[stm::sck::synchronize::Y_N1]);

		// partial binding
		(*sync_step_mf)[smf::sck::synchronize::X_N1] = (*front_agc   )[mlt::sck::imultiply  ::Z_N ];
		(*sync_step_mf)[smf::sck::synchronize::DEL ] = (*feedbr      )[fbr::sck::produce    ::Y_N ];
		(*feedbr      )[fbr::sck::memorize   ::X_N ] = (*sync_frame  )[sfm::sck::synchronize::DEL ];
		(*sync_timing )[stm::sck::extract    ::B_N1] = (*sync_step_mf)[smf::sck::synchronize::B_N1];
		(*sync_timing )[stm::sck::extract    ::Y_N1] = (*sync_step_mf)[smf::sck::synchronize::Y_N1];

		std::vector<runtime::Task*> firsts_wl12 = { &(*radio )[rad::tsk::receive], &(*feedbr)[fbr::tsk::produce] };

		std::vector<runtime::Task*> lasts_wl12 = { &(*feedbr)[fbr::tsk::memorize] };
		std::vector<runtime::Task*> exclude_wl12 = { &(*pl_scrambler)[scr::tsk::descramble] };

		runtime::Sequence sequence_waiting_and_learning_1_2(firsts_wl12, lasts_wl12, exclude_wl12);
		sequence_waiting_and_learning_1_2.set_auto_stop(false);

		if (enable_logs)
		{
			std::ofstream fs1("rx_sequence_waiting_and_learning_1_2.dot");
			sequence_waiting_and_learning_1_2.export_dot(fs1);
		}

		// id of the current frame
		unsigned m = 0;

		// #################"
		// # WAITING PHASE #"
		// #################"

#ifdef DVBS2_LINK_UHD
		const int radio_flush_period = params.n_frames * 100;
		auto radio_usrp = dynamic_cast<Radio_USRP<>*>(radio.get());
#endif
		sync_coarse_f->set_PLL_coeffs(1, 1/std::sqrt(2.0), 1e-4);
		sequence_waiting_and_learning_1_2.exec([&](const std::vector<const int*>& statuses)
		{
			m += params.n_frames;
			if (statuses.back() == nullptr && enable_logs)
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
		sync_frame  ->reset();

		auto end_waiting = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds_waiting = end_waiting - start_waiting;
		std::cout << "Done (" << elapsed_seconds_waiting.count() << "s)." << std::endl;

		// ============================================================================================================
		// LEARNING PHASE 1 & 2 =======================================================================================
		// ============================================================================================================
		auto start_learning = std::chrono::system_clock::now();
		std::cout << "Learning phase... ";
		std::cout.flush();

		// ####################
		// # LEARNING PHASE 1 #
		// ####################

		m = 0;
		int limit = 150;
		sync_coarse_f->set_PLL_coeffs(1, 1/std::sqrt(2.0), 1e-4);
		sequence_waiting_and_learning_1_2.exec([&](const std::vector<const int*>& statuses)
		{
			m += params.n_frames;
			if (statuses.back() == nullptr && enable_logs)
				std::clog << rang::tag::warning << "Sequence aborted! (learning phase 1&2, m = " << m << ")"
				          << std::endl;

			if (limit == 150 && m >= 150)
			{
				// ####################
				// # LEARNING PHASE 2 #
				// ####################
				limit = m + 150;
				sync_coarse_f->set_PLL_coeffs(1, 1/std::sqrt(2.0), 5e-5);
			}
			return m >= limit;
		});

		// ============================================================================================================
		// LEARNING PHASE 3 ===========================================================================================
		// ============================================================================================================
		// partial unbinding
		(*sync_step_mf)[smf::sck::synchronize::X_N1].unbind((*front_agc   )[mlt::sck::imultiply  ::Z_N ]);
		(*feedbr      )[fbr::sck::memorize   ::X_N ].unbind((*sync_frame  )[sfm::sck::synchronize::DEL ]);
		(*sync_timing )[stm::sck::extract    ::B_N1].unbind((*sync_step_mf)[smf::sck::synchronize::B_N1]);
		(*sync_timing )[stm::sck::extract    ::Y_N1].unbind((*sync_step_mf)[smf::sck::synchronize::Y_N1]);

		// partial binding
		(*sync_coarse_f)[sfc::sck::synchronize::X_N1] = (*front_agc  )[mlt::sck::imultiply  ::Z_N ];
		(*sync_timing  )[stm::sck::extract    ::B_N1] = (*sync_timing)[stm::sck::synchronize::B_N1];
		(*sync_timing  )[stm::sck::extract    ::Y_N1] = (*sync_timing)[stm::sck::synchronize::Y_N1];

		std::vector<runtime::Task*> firsts_l3 = { &(*radio)[rad::tsk::receive] };

		std::vector<runtime::Task*> lasts_l3 = { &(*sync_fine_pf)[sff::tsk::synchronize] };

		runtime::Sequence sequence_learning_3(firsts_l3, lasts_l3);
		sequence_learning_3.set_auto_stop(false);

		if (enable_logs)
		{
			std::ofstream fs2("rx_sequence_learning_3.dot");
			sequence_learning_3.export_dot(fs2);
		}

		// ####################
		// # LEARNING PHASE 3 #
		// ####################

		limit = m + 200;
		sequence_learning_3.exec([&](const std::vector<const int*>& statuses)
		{
			m += params.n_frames;
			if (statuses.back() == nullptr && enable_logs)
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

	sync_timing->set_act(true);

	// ######################
	// # TRANSMISSION PHASE #
	// ######################

	// reset the statistics of the tasks before the transmission phase
	for (auto& type : tasks_per_types)
		for (auto& tsk : type)
			tsk->reset();

#ifdef MULTI_THREADED
	pipeline_transmission.bind_adaptors();
	pipeline_transmission.exec([] (const std::vector<const int*>& statuses) { return false; });
#else
	// start the transmission sequence
	sequence_transmission.exec([]
		(const std::vector<const int*>& statuses)
		{
			if (statuses.back() == nullptr && enable_logs)
				std::clog << std::endl << rang::tag::warning << "Sequence aborted! (transmission phase)" << std::endl;
			return tools::Terminal::is_interrupt();
		});
#endif /* MULTI_THREADED */

#ifdef DVBS2_LINK_UHD
	// stop the radio thread
	if (radio_usrp != nullptr)
		radio_usrp->cancel_waiting();
#endif

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
			tools::Stats::show(stages[ss]->get_tasks_per_types(), ordered, true, stats_out);
		}
#else
		stats_out << "#" << std::endl << "# Sequence sequential (" << sequence_transmission.get_n_threads()
		          << " thread(s)): " << std::endl;
		tools::Stats::show(sequence_transmission.get_tasks_per_types(), ordered, true, stats_out);
#endif /* MULTI_THREADED */
	}

	std::cout << "#" << std::endl;
	std::cout << "# End of the simulation" << std::endl;

#ifdef MULTI_THREADED
	if (thread_pinnig)
		tools::Thread_pinning::destroy();
#endif /* MULTI_THREADED */

	return EXIT_SUCCESS;
}

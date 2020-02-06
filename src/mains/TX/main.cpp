#include <aff3ct.hpp>

#include "version.h"
#include "Factory/DVBS2O/DVBS2O.hpp"

using namespace aff3ct;

#define MULTI_THREADED // comment this line to disable multi-threaded TX

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
	}
#endif

	// get the parameter to configure the tools and modules
	auto params = factory::DVBS2O(argc, argv);

	std::map<std::string,tools::header_list> headers;
	std::vector<factory::Factory*> param_vec;
	param_vec.push_back(&params);
	tools::Header::print_parameters(param_vec, true, std::cout);

	std::vector<float> shaping_in  ((params.pl_frame_size) * 2, 0.0f);

	// build a terminal just for signal handling
	std::vector<std::unique_ptr<tools::Reporter>> reporters;
	std::unique_ptr<tools::Terminal> terminal;
	tools::Sigma<> noise;

	const auto ebn0 = 3.8f;
	const float rate = (float)params.K_bch / (float)params.N_ldpc;
	const auto esn0  = tools::ebn0_to_esn0 (ebn0, rate, params.bps);
	const auto sigma = tools::esn0_to_sigma(esn0);
	noise.set_values(sigma, ebn0, esn0);
	reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_noise<>(noise))); // report the noise values (Es/N0 and Eb/N0)
	terminal = std::unique_ptr<tools::Terminal>(new tools::Terminal_std(reporters));

	// construct tools
	std::unique_ptr<tools::Constellation<R>> cstl(new tools::Constellation_user<R>(params.constellation_file));
	std::unique_ptr<tools::Interleaver_core<>> itl_core(factory::DVBS2O::build_itl_core<>(params));
	tools::BCH_polynomial_generator<B > poly_gen(params.N_bch_unshortened, 12, params.bch_prim_poly);

	// construct modules
	std::unique_ptr<module::Source<>        > source        (factory::DVBS2O::build_source <>     (params, 0         ));
	std::unique_ptr<module::Scrambler<>     > bb_scrambler  (factory::DVBS2O::build_bb_scrambler<>(params            ));
	std::unique_ptr<module::Encoder<>       > BCH_encoder   (factory::DVBS2O::build_bch_encoder <>(params, poly_gen  ));
	std::unique_ptr<tools ::Codec<>         > LDPC_cdc      (factory::DVBS2O::build_ldpc_cdc    <>(params            ));
	std::unique_ptr<module::Interleaver<>   > itl           (factory::DVBS2O::build_itl         <>(params, *itl_core ));
	std::unique_ptr<module::Modem<>         > modem         (factory::DVBS2O::build_modem       <>(params, cstl.get()));
	std::unique_ptr<module::Framer<>        > framer        (factory::DVBS2O::build_framer      <>(params            ));
	std::unique_ptr<module::Scrambler<float>> pl_scrambler  (factory::DVBS2O::build_pl_scrambler<>(params            ));
	std::unique_ptr<module::Filter<>        > shaping_filter(factory::DVBS2O::build_uprrc_filter<>(params            ));
	std::unique_ptr<module::Radio<>         > radio         (factory::DVBS2O::build_radio<>       (params            ));

	auto* LDPC_encoder = &LDPC_cdc->get_encoder();

#ifdef MULTI_THREADED
	std::unique_ptr<module::Adaptor_1_to_n> adaptor_1_to_n(new module::Adaptor_1_to_n(params.K_bch,             typeid(int  ), 1, active_waiting, params.n_frames));
	std::unique_ptr<module::Adaptor_n_to_1> adaptor_n_to_1(new module::Adaptor_n_to_1(2 * params.pl_frame_size, typeid(float), 1, active_waiting, params.n_frames));
#endif

	BCH_encoder   ->set_custom_name("Encoder BCH" );
	LDPC_encoder  ->set_custom_name("Encoder LDPC");
#ifdef MULTI_THREADED
	adaptor_1_to_n->set_custom_name("Adp_1_to_n"  );
	adaptor_n_to_1->set_custom_name("Adp_n_to_1"  );
#endif

	// the list of the allocated modules for the simulation
	std::vector<const module::Module*> modules;

	modules = { radio         .get(), source        .get(), bb_scrambler  .get(), BCH_encoder   .get(),
	            LDPC_encoder        , itl           .get(), modem         .get(), framer        .get(),
	            pl_scrambler  .get(), shaping_filter.get(),
#ifdef MULTI_THREADED
	            adaptor_1_to_n.get(), adaptor_n_to_1.get()
#endif
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

			// enable the fast mode (= disable the useless verifs in the tasks) if there is no debug and stats modes
			ta->set_fast(false);
		}

	using namespace module;

#ifdef MULTI_THREADED
	(*adaptor_1_to_n)[adp::sck::push_1    ::in1 ].bind((*source        )[src::sck::generate  ::U_K ]);
	(*bb_scrambler  )[scr::sck::scramble  ::X_N1].bind((*adaptor_1_to_n)[adp::sck::pull_n    ::out1]);
	(*BCH_encoder   )[enc::sck::encode    ::U_K ].bind((*bb_scrambler  )[scr::sck::scramble  ::X_N2]);
	(*LDPC_encoder  )[enc::sck::encode    ::U_K ].bind((*BCH_encoder   )[enc::sck::encode    ::X_N ]);
	(*itl           )[itl::sck::interleave::nat ].bind((*LDPC_encoder  )[enc::sck::encode    ::X_N ]);
	(*modem         )[mdm::sck::modulate  ::X_N1].bind((*itl           )[itl::sck::interleave::itl ]);
	(*framer        )[frm::sck::generate  ::Y_N1].bind((*modem         )[mdm::sck::modulate  ::X_N2]);
	(*pl_scrambler  )[scr::sck::scramble  ::X_N1].bind((*framer        )[frm::sck::generate  ::Y_N2]);
	(*adaptor_n_to_1)[adp::sck::push_n    ::in1 ].bind((*pl_scrambler  )[scr::sck::scramble  ::X_N2]);
	(*shaping_filter)[flt::sck::filter    ::X_N1].bind((*adaptor_n_to_1)[adp::sck::pull_1    ::out1]);
	(*radio         )[rad::sck::send      ::X_N1].bind((*shaping_filter)[flt::sck::filter    ::Y_N2]);

	// create a chain per pipeline stage
	tools::Chain chain_stage0  ((*source        )[src::tsk::generate], (*adaptor_1_to_n)[adp::tsk::push_1], 1);
	tools::Chain chain_parallel((*adaptor_1_to_n)[adp::tsk::pull_n  ], (*adaptor_n_to_1)[adp::tsk::push_n], 4);
	tools::Chain chain_stage1  ((*adaptor_n_to_1)[adp::tsk::pull_1  ], (*radio         )[rad::tsk::send  ], 1);

	std::vector<tools::Chain*> chain_stages = { &chain_stage0, &chain_stage1 };

	// DEBUG
	std::ofstream f("chain_parallel.dot");
	chain_parallel.export_dot(f);
	for (size_t cs = 0; cs < chain_stages.size(); cs++)
	{
		std::ofstream fs("chain_stage" + std::to_string(cs) + ".dot");
		chain_stages[cs]->export_dot(fs);
	}

	// enable thread pinning
	chain_parallel.set_thread_pinning(true, { 0, 1, 2, 5 });
	chain_stage0  .set_thread_pinning(true, { 4          });
	chain_stage1  .set_thread_pinning(true, { 6          });

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
	for (auto &cs : chain_stages)
		threads.push_back(std::thread([cs, &terminal, &stop_threads]() {
			cs->exec([&terminal]() { return terminal->is_interrupt(); } );
			stop_threads();
		}));

	// start the parallel chain
	chain_parallel.exec([&terminal]()
	{
		return terminal->is_interrupt();
	});
	stop_threads();

	// wait all the pipeline threads here
	for (auto &t : threads)
		t.join();
#else
	(*bb_scrambler  )[scr::sck::scramble  ::X_N1].bind((*source        )[src::sck::generate  ::U_K ]);
	(*BCH_encoder   )[enc::sck::encode    ::U_K ].bind((*bb_scrambler  )[scr::sck::scramble  ::X_N2]);
	(*LDPC_encoder  )[enc::sck::encode    ::U_K ].bind((*BCH_encoder   )[enc::sck::encode    ::X_N ]);
	(*itl           )[itl::sck::interleave::nat ].bind((*LDPC_encoder  )[enc::sck::encode    ::X_N ]);
	(*modem         )[mdm::sck::modulate  ::X_N1].bind((*itl           )[itl::sck::interleave::itl ]);
	(*framer        )[frm::sck::generate  ::Y_N1].bind((*modem         )[mdm::sck::modulate  ::X_N2]);
	(*pl_scrambler  )[scr::sck::scramble  ::X_N1].bind((*framer        )[frm::sck::generate  ::Y_N2]);
	(*shaping_filter)[flt::sck::filter    ::X_N1].bind((*pl_scrambler  )[scr::sck::scramble  ::X_N2]);
	(*radio         )[rad::sck::send      ::X_N1].bind((*shaping_filter)[flt::sck::filter    ::Y_N2]);

	tools::Chain chain_sequential((*source)[src::tsk::generate], (*radio)[rad::tsk::send]);
	std::ofstream f("chain_sequential.dot");
	chain_sequential.export_dot(f);

	// start the sequentiel chain
	chain_sequential.exec([&terminal]()
	{
		return terminal->is_interrupt();
	});

	// stop the radio thread
	for (auto &m : chain_sequential.get_modules<tools::Interface_waiting>())
		m->cancel_waiting();
#endif

	if (params.stats)
	{
		std::vector<const module::Module*> modules_stats(modules.size());
		for (size_t m = 0; m < modules.size(); m++)
			modules_stats.push_back(modules[m]);

		// std::cout << "#" << std::endl;
		// tools::Stats::show(modules_stats, ordered);
		const auto ordered = true;
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
		std::cout << "#" << std::endl << "# Chain sequential (" << chain_sequential.get_n_threads() << " thread(s)): "
		          << std::endl;
		tools::Stats::show(chain_sequential.get_tasks_per_types(), ordered);
#endif
	}

#ifdef MULTI_THREADED
	if (thread_pinnig)
		tools::Thread_pinning::destroy();
#endif

	return EXIT_SUCCESS;
}

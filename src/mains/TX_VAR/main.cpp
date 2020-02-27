#include <aff3ct.hpp>

#include "version.h"
#include "Factory/DVBS2O/DVBS2O.hpp"

using namespace aff3ct;
using namespace aff3ct::module;

#define MULTI_THREADED // comment this line to disable multi-threaded TX

// global parameters
constexpr bool enable_logs = true;
#ifdef MULTI_THREADED
const bool thread_pinnig = true;
const bool active_waiting = false;
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
	}
#endif

	// install signal handlers
	tools::Terminal::init();

	// get the parameter to configure the tools and modules
	auto params = factory::DVBS2O(argc, argv);

	std::map<std::string,tools::header_list> headers;
	std::vector<factory::Factory*> param_vec;
	param_vec.push_back(&params);
	tools::Header::print_parameters(param_vec, true, std::cout);

	// construct tools
	uptr<tools::Constellation<R>> cstl(new tools::Constellation_user<R>(params.constellation_file));
	uptr<tools::Interleaver_core<>> itl_core(factory::DVBS2O::build_itl_core<>(params));
	tools::BCH_polynomial_generator<B> poly_gen(params.N_bch_unshortened, 12, params.bch_prim_poly);

	// construct modules
	uptr<Source<>                  > source        (factory::DVBS2O::build_source      <>(params, 0         ));
	uptr<Scrambler<>               > bb_scrambler  (factory::DVBS2O::build_bb_scrambler<>(params            ));
	uptr<Encoder<>                 > BCH_encoder   (factory::DVBS2O::build_bch_encoder <>(params, poly_gen  ));
	uptr<tools::Codec<>            > LDPC_cdc      (factory::DVBS2O::build_ldpc_cdc    <>(params            ));
	uptr<Interleaver<>             > itl           (factory::DVBS2O::build_itl         <>(params, *itl_core ));
	uptr<Modem<>                   > modem         (factory::DVBS2O::build_modem       <>(params, cstl.get()));
	uptr<Framer<>                  > framer        (factory::DVBS2O::build_framer      <>(params            ));
	uptr<Scrambler<float>          > pl_scrambler  (factory::DVBS2O::build_pl_scrambler<>(params            ));
	uptr<Filter<>                  > shaping_filter(factory::DVBS2O::build_uprrc_filter<>(params            ));
	uptr<Multiplier_fading_DVBS2O<>> fad_mlt       (factory::DVBS2O::build_fading_mult <>(params            ));
	uptr<Radio<>                   > radio         (factory::DVBS2O::build_radio       <>(params            ));

	auto* LDPC_encoder = &LDPC_cdc->get_encoder();

#ifdef MULTI_THREADED
	// create the adaptors to manage the pipeline in multi-threaded mode
	Adaptor_1_to_n adaptor_1_to_n(params.K_bch,
	                              typeid(int),
	                              1,
	                              active_waiting,
	                              params.n_frames);
	Adaptor_n_to_1 adaptor_n_to_1(2 * params.pl_frame_size,
	                              typeid(float),
	                              1,
	                              active_waiting,
	                              params.n_frames);
#endif

	// add custom name to some modules
	BCH_encoder   ->set_custom_name("Encoder BCH" );
	LDPC_encoder  ->set_custom_name("Encoder LDPC");
#ifdef MULTI_THREADED
	adaptor_1_to_n. set_custom_name("Adp_1_to_n"  );
	adaptor_n_to_1. set_custom_name("Adp_n_to_1"  );
#endif

	// the list of the allocated modules for the simulation
	std::vector<const Module*> modules;
	modules = { /* standard modules */
	            radio  .get(), source.get(), bb_scrambler.get(), BCH_encoder .get(), LDPC_encoder,
	            itl    .get(), modem .get(), framer      .get(), pl_scrambler.get(), shaping_filter.get(),
	            fad_mlt.get(),
#ifdef MULTI_THREADED
	            /* adaptors */
	            &adaptor_1_to_n, &adaptor_n_to_1
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

			if (!ta->is_debug() && !ta->is_stats())
				ta->set_fast(true);
		}

#ifdef MULTI_THREADED
	  adaptor_1_to_n [adp::sck::push_1    ::in1 ].bind((*source        )[src::sck::generate  ::U_K ]);
	(*bb_scrambler  )[scr::sck::scramble  ::X_N1].bind(  adaptor_1_to_n [adp::sck::pull_n    ::out1]);
	(*BCH_encoder   )[enc::sck::encode    ::U_K ].bind((*bb_scrambler  )[scr::sck::scramble  ::X_N2]);
	(*LDPC_encoder  )[enc::sck::encode    ::U_K ].bind((*BCH_encoder   )[enc::sck::encode    ::X_N ]);
	(*itl           )[itl::sck::interleave::nat ].bind((*LDPC_encoder  )[enc::sck::encode    ::X_N ]);
	(*modem         )[mdm::sck::modulate  ::X_N1].bind((*itl           )[itl::sck::interleave::itl ]);
	(*framer        )[frm::sck::generate  ::Y_N1].bind((*modem         )[mdm::sck::modulate  ::X_N2]);
	(*pl_scrambler  )[scr::sck::scramble  ::X_N1].bind((*framer        )[frm::sck::generate  ::Y_N2]);
	  adaptor_n_to_1 [adp::sck::push_n    ::in1 ].bind((*pl_scrambler  )[scr::sck::scramble  ::X_N2]);
	(*shaping_filter)[flt::sck::filter    ::X_N1].bind(  adaptor_n_to_1 [adp::sck::pull_1    ::out1]);
	(*fad_mlt       )[mlt::sck::imultiply ::X_N ].bind((*shaping_filter)[flt::sck::filter    ::Y_N2]);
	(*radio         )[rad::sck::send      ::X_N1].bind((*fad_mlt       )[mlt::sck::imultiply ::Z_N ]);

	// create a chain per pipeline stage
	tools::Chain chain_stage0         ((*source        )[src::tsk::generate], 1, thread_pinnig, { 4          });
	tools::Chain chain_stage1_parallel(  adaptor_1_to_n [adp::tsk::pull_n  ], 4, thread_pinnig, { 0, 1, 2, 5 });
	tools::Chain chain_stage2         (  adaptor_n_to_1 [adp::tsk::pull_1  ], 1, thread_pinnig, { 6          });

	std::vector<tools::Chain*> chain_stages = { &chain_stage0, &chain_stage1_parallel, &chain_stage2 };

	// dump the chains in dot format
	for (size_t cs = 0; cs < chain_stages.size() && enable_logs; cs++)
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

	// start the pipeline threads
	std::vector<std::thread> threads;
	for (auto &cs : chain_stages)
		threads.push_back(std::thread([cs, &stop_threads]() {
			cs->exec([]() { return tools::Terminal::is_interrupt(); } );
			stop_threads();
		}));

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

	tools::Chain chain_sequential((*source)[src::tsk::generate]);
	if (enable_logs)
	{
		std::ofstream f("chain_sequential.dot");
		chain_sequential.export_dot(f);
	}

	// start the sequential chain
	chain_sequential.exec([]() { return tools::Terminal::is_interrupt(); });

	// stop the radio thread
	for (auto &m : chain_sequential.get_modules<tools::Interface_waiting>())
		m->cancel_waiting();
#endif

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

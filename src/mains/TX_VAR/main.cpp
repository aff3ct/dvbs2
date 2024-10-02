#include <fstream>
#include <aff3ct.hpp>
#include <streampu.hpp>

#include "Factory/DVBS2/DVBS2.hpp"
#ifdef DVBS2_LINK_UHD
#include "Module/Radio/Radio_USRP/Radio_USRP.hpp"
#endif

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
		spu::tools::Thread_pinning::init();
		// tools::Thread_pinning::set_logs(enable_logs);
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
	uptr<tools::Constellation<R>> cstl(new tools::Constellation_user<R>(params.constellation_file));
	uptr<tools::Interleaver_core<>> itl_core(factory::DVBS2::build_itl_core<>(params));
	tools::BCH_polynomial_generator<B> poly_gen(params.N_bch_unshortened, 12, params.bch_prim_poly);

	// construct modules
	uptr<spu::module::Source<>    > source        (factory::DVBS2::build_source      <>(params, 0         ));
	uptr<Scrambler<>              > bb_scrambler  (factory::DVBS2::build_bb_scrambler<>(params            ));
	uptr<Encoder<>                > BCH_encoder   (factory::DVBS2::build_bch_encoder <>(params, poly_gen  ));
	uptr<tools::Codec<>           > LDPC_cdc      (factory::DVBS2::build_ldpc_cdc    <>(params            ));
	uptr<Interleaver<>            > itl           (factory::DVBS2::build_itl         <>(params, *itl_core ));
	uptr<Modem<>                  > modem         (factory::DVBS2::build_modem       <>(params, cstl.get()));
	uptr<Framer<>                 > framer        (factory::DVBS2::build_framer      <>(params            ));
	uptr<Scrambler<float>         > pl_scrambler  (factory::DVBS2::build_pl_scrambler<>(params            ));
	uptr<Filter<>                 > shaping_filter(factory::DVBS2::build_uprrc_filter<>(params            ));
	uptr<Multiplier_fading_DVBS2<>> fad_mlt       (factory::DVBS2::build_fading_mult <>(params            ));
	uptr<Radio<>                  > radio         (factory::DVBS2::build_radio       <>(params            ));

	auto* LDPC_encoder = &LDPC_cdc->get_encoder();

	// add custom name to some modules
	BCH_encoder ->set_custom_name("Encoder BCH" );
	LDPC_encoder->set_custom_name("Encoder LDPC");

	// the full transmission chain binding
	(*bb_scrambler  )[scr::sck::scramble  ::X_N1] = (*source        )[spu::module::src::sck::generate  ::out_data];
	(*BCH_encoder   )[enc::sck::encode    ::U_K ] = (*bb_scrambler  )[             scr::sck::scramble  ::X_N2    ];
	(*LDPC_encoder  )[enc::sck::encode    ::U_K ] = (*BCH_encoder   )[             enc::sck::encode    ::X_N     ];
	(*itl           )[itl::sck::interleave::nat ] = (*LDPC_encoder  )[             enc::sck::encode    ::X_N     ];
	(*modem         )[mdm::sck::modulate  ::X_N1] = (*itl           )[             itl::sck::interleave::itl     ];
	(*framer        )[frm::sck::generate  ::Y_N1] = (*modem         )[             mdm::sck::modulate  ::X_N2    ];
	(*pl_scrambler  )[scr::sck::scramble  ::X_N1] = (*framer        )[             frm::sck::generate  ::Y_N2    ];
	(*shaping_filter)[flt::sck::filter    ::X_N1] = (*pl_scrambler  )[             scr::sck::scramble  ::X_N2    ];
	(*fad_mlt       )[mlt::sck::imultiply ::X_N ] = (*shaping_filter)[             flt::sck::filter    ::Y_N2    ];
	(*radio         )[rad::sck::send      ::X_N1] = (*fad_mlt       )[             mlt::sck::imultiply ::Z_N     ];

	// first stages of the whole transmission sequence
	const std::vector<spu::runtime::Task*> firsts_t = { &(*source)[spu::module::src::tsk::generate] };

#ifdef MULTI_THREADED
	// pipeline definition with separation stages
	const std::vector<std::pair<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>> sep_stages =
	{ // pipeline stage 0
	  std::make_pair<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*source)[spu::module::src::tsk::generate] },
	    { &(*source)[spu::module::src::tsk::generate] }),
	  // pipeline stage 1
	  std::make_pair<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*bb_scrambler)[scr::tsk::scramble] },
	    { &(*pl_scrambler)[scr::tsk::scramble] }),
	  // pipeline stage 2
	  std::make_pair<std::vector<spu::runtime::Task*>, std::vector<spu::runtime::Task*>>(
	    { &(*shaping_filter)[flt::tsk::filter] },
	    { }),
	};
	// number of threads per stages
	const std::vector<size_t> n_threads_per_stages = { 1, 6, 1 };
	// synchronization buffer size between stages
	const std::vector<size_t> buffer_sizes(sep_stages.size() -1, 1);
	// type of waiting between stages (true = active, false = passive)
	const std::vector<bool> active_waitings(sep_stages.size() -1, active_waiting);
	// enable thread pinning
	const std::vector<bool> thread_pinnigs(sep_stages.size(), thread_pinnig);
	// process unit (pu) ids per stage for thread pinning
	const std::vector<std::vector<size_t>> puids = { { 4 },                // for stage 0
	                                                 { 0, 1, 2, 5, 7, 8 }, // for stage 1
	                                                 { 6 } };              // for stage 2

	spu::runtime::Pipeline pipeline_transmission(firsts_t, sep_stages, n_threads_per_stages, buffer_sizes,
	                                             active_waitings, thread_pinnigs, puids);
	if (enable_logs)
	{
		std::ofstream f("tx_pipeline_transmission.dot");
		pipeline_transmission.export_dot(f);
	}
	auto tasks_per_types = pipeline_transmission.get_tasks_per_types();
#else
	runtime::Sequence sequence_transmission(firsts_t);
	if (enable_logs)
	{
		std::ofstream f("tx_sequence_transmission.dot");
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

#ifdef MULTI_THREADED
	pipeline_transmission.exec([]() { return false; });
	// no need to stop the radio thread here, it is automatically done by the pipeline
#else
	sequence_transmission.exec([]() { return false; });
	// stop the radio thread
	for (auto &m : sequence_transmission.get_modules<tools::Interface_waiting>())
		m->cancel_waiting();
#endif /* MULTI_THREADED */

#ifdef DVBS2_LINK_UHD
	// stop the radio thread
	auto radio_usrp = dynamic_cast<Radio_USRP<>*>(radio.get());
	if (radio_usrp != nullptr)
		radio_usrp->cancel_waiting();
#endif

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
	}

#ifdef MULTI_THREADED
	if (thread_pinnig)
		spu::tools::Thread_pinning::destroy();
#endif /* MULTI_THREADED */

	return EXIT_SUCCESS;
}

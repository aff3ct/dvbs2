#include <vector>
#include <numeric>
#include <iostream>

#include <aff3ct.hpp>

#include "Scrambler/Scrambler_BB/Scrambler_BB.hpp"
#include "Params_DVBS2O/Params_DVBS2O.hpp"
#include "Factory_DVBS2O/Factory_DVBS2O.hpp"
#include "Framer/Framer.hpp"
#include "Filter/Filter_UPFIR/Filter_UPRRC/Filter_UPRRC_ccr_naive.hpp"
#include "Scrambler/Scrambler_PL/Scrambler_PL.hpp"
#include "Sink/Sink.hpp"

#ifdef _OPENMP
#include <omp.h>
#else
inline int omp_get_max_threads() { return 1; }
#endif

using namespace aff3ct;

int main(int argc, char** argv)
{
	// get the number of available threads from OpenMP
	const int n_threads = omp_get_max_threads();

	// get the parameter to configure the tools and modules
	const auto params = Params_DVBS2O(argc, argv);

	// declare vectors of module for multi-threaded Monte-Carlo simulation
	std::vector<std::unique_ptr<module::Source<>                   >> source      (n_threads);
	std::vector<std::unique_ptr<module::Scrambler<>                >> bb_scrambler(n_threads);
	std::vector<std::unique_ptr<module::Encoder<>                  >> BCH_encoder (n_threads);
	std::vector<std::unique_ptr<module::Decoder_HIHO<>             >> BCH_decoder (n_threads);
	std::vector<std::unique_ptr<module::Codec_SIHO<>               >> LDPC_cdc    (n_threads);
	std::vector<std::unique_ptr<module::Interleaver<>              >> itl_tx      (n_threads);
	std::vector<std::unique_ptr<module::Interleaver<float,uint32_t>>> itl_rx      (n_threads);
	std::vector<std::unique_ptr<module::Modem<>                    >> modem       (n_threads);
	std::vector<std::unique_ptr<module::Channel<>                  >> channel     (n_threads);
	std::vector<std::unique_ptr<module::Framer<>                   >> framer      (n_threads);
	std::vector<std::unique_ptr<module::Scrambler<float>           >> pl_scrambler(n_threads);
	std::vector<std::unique_ptr<module::Monitor_BFER<B>            >> monitor     (n_threads);

	// construct common tools
	std::unique_ptr<tools::Interleaver_core        < >> itl_core(Factory_DVBS2O::build_itl_core<>(params));
	                tools::BCH_polynomial_generator<B > poly_gen(params.N_BCH_unshortened, 12            );

	// generate the seeds
	std::vector<std::vector<int>> seeds(n_threads, std::vector<int>(2));
	for (int t = 0; t < n_threads; t++)
		std::iota(seeds[t].begin(), seeds[t].end(), t * 2);

	// need to parallelize this loop in order to allocate the data on the right NUMA memory bank
#pragma omp parallel for schedule(static, 1)
	for (int t = 0; t < n_threads; t++)
	{
		// construct specific tools
		std::unique_ptr<tools::Constellation<R>> cstl(new tools::Constellation_user<R>(params.constellation_file));

		// construct modules
		source      [t] = std::unique_ptr<module::Source<>                   >(Factory_DVBS2O::build_source           <>(params, seeds[t][0]    ));
		bb_scrambler[t] = std::unique_ptr<module::Scrambler<>                >(Factory_DVBS2O::build_bb_scrambler     <>(params                 ));
		BCH_encoder [t] = std::unique_ptr<module::Encoder<>                  >(Factory_DVBS2O::build_bch_encoder      <>(params, poly_gen       ));
		BCH_decoder [t] = std::unique_ptr<module::Decoder_HIHO<>             >(Factory_DVBS2O::build_bch_decoder      <>(params, poly_gen       ));
		LDPC_cdc    [t] = std::unique_ptr<module::Codec_SIHO<>               >(Factory_DVBS2O::build_ldpc_cdc         <>(params                 ));
		itl_tx      [t] = std::unique_ptr<module::Interleaver<>              >(Factory_DVBS2O::build_itl              <>(params, *itl_core      ));
		itl_rx      [t] = std::unique_ptr<module::Interleaver<float,uint32_t>>(Factory_DVBS2O::build_itl<float,uint32_t>(params, *itl_core      ));
		modem       [t] = std::unique_ptr<module::Modem<>                    >(Factory_DVBS2O::build_modem            <>(params, std::move(cstl)));
		channel     [t] = std::unique_ptr<module::Channel<>                  >(Factory_DVBS2O::build_channel          <>(params, seeds[t][1]    ));
		framer      [t] = std::unique_ptr<module::Framer<>                   >(Factory_DVBS2O::build_framer           <>(params                 ));
		pl_scrambler[t] = std::unique_ptr<module::Scrambler<float>           >(Factory_DVBS2O::build_pl_scrambler     <>(params                 ));
		monitor     [t] = std::unique_ptr<module::Monitor_BFER<B>            >(Factory_DVBS2O::build_monitor          <>(params                 ));

		auto& LDPC_encoder = LDPC_cdc[t]->get_encoder();
		auto& LDPC_decoder = LDPC_cdc[t]->get_decoder_siho();

		LDPC_encoder  ->set_short_name("LDPC Encoder");
		LDPC_decoder  ->set_short_name("LDPC Decoder");
		BCH_encoder[t]->set_short_name("BCH Encoder" );
		BCH_decoder[t]->set_short_name("BCH Decoder" );
	}

	// construct a common monitor module to reduce all the monitors
	module::Monitor_reduction_M<module::Monitor_BFER<>> monitor_red(monitor);
	module::Monitor_reduction::set_reduce_frequency(std::chrono::milliseconds(500));
	module::Monitor_reduction::check_reducible();

	// create reporters to display results in the terminal
	tools::Sigma<> noise;
	std::vector<tools::Reporter*> reporters =
	{
		new tools::Reporter_noise     <>(noise      ), // report the noise values (Es/N0 and Eb/N0)
		new tools::Reporter_BFER      <>(monitor_red), // report the bit/frame error rates
		new tools::Reporter_throughput<>(monitor_red)  // report the simulation throughputs
	};
	// convert the vector of reporter pointers into a vector of smart pointers
	std::vector<std::unique_ptr<tools::Reporter>> reporters_uptr;
	for (auto rep : reporters) reporters_uptr.push_back(std::unique_ptr<tools::Reporter>(rep));

	// create a terminal that will display the collected data from the reporters
	std::unique_ptr<tools::Terminal> terminal(new tools::Terminal_std(reporters_uptr));

	// display the legend in the terminal
	terminal->legend();

	// the module list
	std::vector<std::vector<const module::Module*>> modules(n_threads);

	// need to parallelize this loop in order to allocate the data on the right NUMA memory bank
#pragma omp parallel for schedule(static, 1)
	for (int t = 0; t < n_threads; t++)
	{
		auto& LDPC_encoder = LDPC_cdc[t]->get_encoder();
		auto& LDPC_decoder = LDPC_cdc[t]->get_decoder_siho();

		// build a list of modules
		modules[t] = { bb_scrambler[t].get(), BCH_encoder [t].get(), BCH_decoder[t].get(), LDPC_encoder   .get(),
		               LDPC_decoder   .get(), itl_tx      [t].get(), itl_rx     [t].get(), modem       [t].get(),
		               framer      [t].get(), pl_scrambler[t].get(), source     [t].get(), monitor     [t].get(),
		               channel     [t].get()                                                                     };

		// configuration of the module tasks
		for (auto& m : modules[t])
			for (auto& ta : m->tasks)
			{
				ta->set_autoalloc  (true        ); // enable the automatic allocation of the data in the tasks
				ta->set_autoexec   (false       ); // disable the auto execution mode of the tasks
				ta->set_debug      (false       ); // disable the debug mode
				ta->set_debug_limit(16          ); // display only the 16 first bits if the debug mode is enabled
				ta->set_stats      (params.stats); // enable the statistics

				// enable the fast mode (= disable the useless verifs in the tasks) if there is no debug and stats modes
				ta->set_fast(!ta->is_debug() && !ta->is_stats());
			}
	}

	// initialization
	poly_gen.set_g(params.BCH_gen_poly);
	itl_core->init();

	using namespace module;

	// not very important to parallelize this loop
#pragma omp parallel for schedule(static, 1)
	for (int t = 0; t < n_threads; t++)
	{
		auto& LDPC_encoder = LDPC_cdc[t]->get_encoder();
		auto& LDPC_decoder = LDPC_cdc[t]->get_decoder_siho();

		// socket binding
		(*bb_scrambler[t])[scr::sck::scramble    ::X_N1].bind((*source      [t])[src::sck::generate    ::U_K ]);
		(*BCH_encoder [t])[enc::sck::encode      ::U_K ].bind((*bb_scrambler[t])[scr::sck::scramble    ::X_N2]);
		(*LDPC_encoder   )[enc::sck::encode      ::U_K ].bind((*BCH_encoder [t])[enc::sck::encode      ::X_N ]);
		(*itl_tx      [t])[itl::sck::interleave  ::nat ].bind((*LDPC_encoder   )[enc::sck::encode      ::X_N ]);
		(*modem       [t])[mdm::sck::modulate    ::X_N1].bind((*itl_tx      [t])[itl::sck::interleave  ::itl ]);
		(*framer      [t])[frm::sck::generate    ::Y_N1].bind((*modem       [t])[mdm::sck::modulate    ::X_N2]);
		(*pl_scrambler[t])[scr::sck::scramble    ::X_N1].bind((*framer      [t])[frm::sck::generate    ::Y_N2]);
		(*channel     [t])[chn::sck::add_noise   ::X_N ].bind((*pl_scrambler[t])[scr::sck::scramble    ::X_N2]);
		(*pl_scrambler[t])[scr::sck::descramble  ::Y_N1].bind((*channel     [t])[chn::sck::add_noise   ::Y_N ]);
		(*framer      [t])[frm::sck::remove_plh  ::Y_N1].bind((*pl_scrambler[t])[scr::sck::descramble  ::Y_N2]);
		(*modem       [t])[mdm::sck::demodulate  ::Y_N1].bind((*framer      [t])[frm::sck::remove_plh  ::Y_N2]);
		(*itl_rx      [t])[itl::sck::deinterleave::itl ].bind((*modem       [t])[mdm::sck::demodulate  ::Y_N2]);
		(*LDPC_decoder   )[dec::sck::decode_siho ::Y_N ].bind((*itl_rx      [t])[itl::sck::deinterleave::nat ]);
		(*BCH_decoder [t])[dec::sck::decode_hiho ::Y_N ].bind((*LDPC_decoder   )[dec::sck::decode_siho ::V_K ]);
		(*bb_scrambler[t])[scr::sck::descramble  ::Y_N1].bind((*BCH_decoder [t])[dec::sck::decode_hiho ::V_K ]);
		(*monitor     [t])[mnt::sck::check_errors::U   ].bind((*source      [t])[src::sck::generate    ::U_K ]);
		(*monitor     [t])[mnt::sck::check_errors::V   ].bind((*bb_scrambler[t])[scr::sck::descramble  ::Y_N2]);

		// reset the memory of the decoder after the end of each communication
		monitor[t]->add_handler_check(std::bind(&module::Decoder::reset, LDPC_decoder));
	}

	// a loop over the various SNRs
	for (auto ebn0 = params.ebn0_min; ebn0 < params.ebn0_max; ebn0 += params.ebn0_step)
	{
		// compute the code rate
		const float R = (float)params.K_BCH / (float)params.N_LDPC;

		// compute the current sigma for the channel noise
		const auto esn0  = tools::ebn0_to_esn0 (ebn0, R, params.BPS);
		const auto sigma = tools::esn0_to_sigma(esn0);

		noise.set_noise(sigma, ebn0, esn0);

		for (int t = 0; t < n_threads; t++)
		{
			// update the sigma of the modem and the channel
			LDPC_cdc[t]->set_noise(noise);
			modem   [t]->set_noise(noise);
			channel [t]->set_noise(noise);
		}

		// display the performance (BER and FER) in real time (in a separate thread)
		terminal->start_temp_report();

		// very important to parallelize this loop, this is the compute intensive loop!
#pragma omp parallel for schedule(static, 1)
		for (int t = 0; t < n_threads; t++)
		{
			auto& LDPC_encoder = LDPC_cdc[t].get()->get_encoder();
			auto& LDPC_decoder = LDPC_cdc[t].get()->get_decoder_siho();

			// tasks execution
			while (!module::Monitor_reduction::is_done_all() && !terminal->is_interrupt())
			{
				(*source      [t])[src::tsk::generate    ].exec();
				(*bb_scrambler[t])[scr::tsk::scramble    ].exec();
				(*BCH_encoder [t])[enc::tsk::encode      ].exec();
				(*LDPC_encoder   )[enc::tsk::encode      ].exec();
				(*itl_tx      [t])[itl::tsk::interleave  ].exec();
				(*modem       [t])[mdm::tsk::modulate    ].exec();
				(*framer      [t])[frm::tsk::generate    ].exec();
				(*pl_scrambler[t])[scr::tsk::scramble    ].exec();
				(*channel     [t])[chn::tsk::add_noise   ].exec();
				(*pl_scrambler[t])[scr::tsk::descramble  ].exec();
				(*framer      [t])[frm::tsk::remove_plh  ].exec();
				(*modem       [t])[mdm::tsk::demodulate  ].exec();
				(*itl_rx      [t])[itl::tsk::deinterleave].exec();
				(*LDPC_decoder   )[dec::tsk::decode_siho ].exec();
				(*BCH_decoder [t])[dec::tsk::decode_hiho ].exec();
				(*bb_scrambler[t])[scr::tsk::descramble  ].exec();
				(*monitor     [t])[mnt::tsk::check_errors].exec();
			}
		}

		// final reduction
		const bool fully = true, final = true;
		module::Monitor_reduction::is_done_all(fully, final);

		// display the performance (BER and FER) in the terminal
		terminal->final_report();

		// if user pressed Ctrl+c twice, exit the SNRs loop
		if (terminal->is_over()) break;

		// reset the monitors and the terminal for the next SNR
		module::Monitor_reduction::reset_all();
		terminal->reset();

		// display the statistics of the tasks (if enabled)
		if (params.stats)
		{
			std::vector<std::vector<const module::Module*>> modules_stats(modules[0].size());
			for (size_t m = 0; m < modules[0].size(); m++)
				for (int t = 0; t < n_threads; t++)
					modules_stats[m].push_back(modules[t][m]);
			auto ordered = true;
			tools::Stats::show(modules_stats, ordered);
		}
	}

	std::cout << "#" << std::endl;
	std::cout << "# End of the simulation" << std::endl;

	return EXIT_SUCCESS;
}

#include <vector>
#include <numeric>
#include <iostream>
#ifdef _OPENMP
#include <omp.h>
#endif

#include <aff3ct.hpp>

#include "Factory/Factory_DVBS2O/Factory_DVBS2O.hpp"

using namespace aff3ct;

#ifndef _OPENMP
inline int omp_get_thread_num () { return 0; }
inline int omp_get_num_threads() { return 1; }
#endif

namespace aff3ct { namespace module {
using Monitor_BFER_reduction = Monitor_reduction_M<Monitor_BFER<>>;
} }

int main(int argc, char** argv)
{
	// get the parameter to configure the tools and modules
	auto params = Params_DVBS2O(argc, argv);

	// declare shared modules and tools
	std::vector<std::unique_ptr<module::Monitor_BFER<>>>        monitors;
	            std::unique_ptr<module::Monitor_BFER_reduction> monitor_red;
	std::vector<std::unique_ptr<tools ::Reporter>>              reporters;
	            std::unique_ptr<tools ::Terminal>               terminal;
	                            tools ::Sigma<>                 noise;

	// the list of the allocated modules for the simulation
	std::vector<std::vector<const module::Module*>> modules;

#pragma omp parallel
{
	// get the thread id from OpenMP
	const int tid = omp_get_thread_num();

#pragma omp single
{
	// get the number of available threads from OpenMP
	const size_t n_threads = (size_t)omp_get_num_threads();
	monitors.resize(n_threads);
	modules .resize(n_threads);
}
// end of #pragma omp single

	// construct tools
	std::unique_ptr<tools::Constellation           <float>> cstl    (new tools::Constellation_user<float>(params.constellation_file));
	std::unique_ptr<tools::Interleaver_core        <     >> itl_core(Factory_DVBS2O::build_itl_core<>(params));
	                tools::BCH_polynomial_generator<      > poly_gen(params.N_bch_unshortened, 12, params.bch_prim_poly);

	// construct modules
	std::unique_ptr<module::Source<>                   > source      (Factory_DVBS2O::build_source           <>(params, tid*2+0        ));
	std::unique_ptr<module::Scrambler<>                > bb_scrambler(Factory_DVBS2O::build_bb_scrambler     <>(params                 ));
	std::unique_ptr<module::Encoder<>                  > BCH_encoder (Factory_DVBS2O::build_bch_encoder      <>(params, poly_gen       ));
	std::unique_ptr<module::Decoder_HIHO<>             > BCH_decoder (Factory_DVBS2O::build_bch_decoder      <>(params, poly_gen       ));
	std::unique_ptr<module::Codec_SIHO<>               > LDPC_cdc    (Factory_DVBS2O::build_ldpc_cdc         <>(params                 ));
	std::unique_ptr<module::Interleaver<>              > itl_tx      (Factory_DVBS2O::build_itl              <>(params, *itl_core      ));
	std::unique_ptr<module::Interleaver<float,uint32_t>> itl_rx      (Factory_DVBS2O::build_itl<float,uint32_t>(params, *itl_core      ));
	std::unique_ptr<module::Modem<>                    > modem       (Factory_DVBS2O::build_modem            <>(params, std::move(cstl)));
	std::unique_ptr<module::Channel<>                  > channel     (Factory_DVBS2O::build_channel          <>(params, tid*2+1, false));
	std::unique_ptr<module::Framer<>                   > framer      (Factory_DVBS2O::build_framer           <>(params                 ));
	std::unique_ptr<module::Scrambler<float>           > pl_scrambler(Factory_DVBS2O::build_pl_scrambler     <>(params                 ));
	monitors[tid] = std::unique_ptr<module::Monitor_BFER<>>          (Factory_DVBS2O::build_monitor          <>(params                 ));

	auto& monitor = monitors[tid];
	auto& LDPC_encoder = LDPC_cdc->get_encoder();
	auto& LDPC_decoder = LDPC_cdc->get_decoder_siho();

	LDPC_encoder->set_custom_name("LDPC Encoder");
	LDPC_decoder->set_custom_name("LDPC Decoder");
	BCH_encoder ->set_custom_name("BCH Encoder" );
	BCH_decoder ->set_custom_name("BCH Decoder" );

// wait until all the 'monitors' have been allocated in order to allocate the 'monitor_red' object
#pragma omp barrier

#pragma omp single nowait
{
	// allocate a common monitor module to reduce all the monitors
	monitor_red = std::unique_ptr<module::Monitor_BFER_reduction>(new module::Monitor_BFER_reduction(monitors));
	monitor_red->set_reduce_frequency(std::chrono::milliseconds(500));
	monitor_red->check_reducible();

	// allocate reporters to display results in the terminal
	reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_noise     <>(noise       ))); // report the noise values (Es/N0 and Eb/N0)
	reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_BFER      <>(*monitor_red))); // report the bit/frame error rates
	reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_throughput<>(*monitor_red))); // report the simulation throughputs

	// allocate a terminal that will display the collected data from the reporters
	terminal = std::unique_ptr<tools::Terminal>(new tools::Terminal_std(reporters));

	// display the legend in the terminal
	terminal->legend();
}
// end of #pragma omp single

	// fulfill the list of modules
	modules[tid] = { bb_scrambler.get(), BCH_encoder .get(), BCH_decoder.get(), LDPC_encoder.get(),
	                 LDPC_decoder.get(), itl_tx      .get(), itl_rx     .get(), modem       .get(),
	                 framer      .get(), pl_scrambler.get(), source     .get(), monitor     .get(),
	                 channel     .get()                                                             };

	// configuration of the module tasks
	for (auto& m : modules[tid])
		for (auto& ta : m->tasks)
		{
			ta->set_autoalloc  (true        ); // enable the automatic allocation of the data in the tasks
			ta->set_autoexec   (false       ); // disable the auto execution mode of the tasks
			ta->set_debug      (params.debug); // disable the debug mode
			ta->set_debug_limit(16          ); // display only the 16 first bits if the debug mode is enabled
			ta->set_stats      (params.stats); // enable the statistics

			// enable the fast mode (= disable the useless verifs in the tasks) if there is no debug and stats modes
			ta->set_fast(false);
		}

	using namespace module;

	// socket binding
	(*bb_scrambler)[scr::sck::scramble    ::X_N1].bind((*source      )[src::sck::generate    ::U_K ]);
	(*BCH_encoder )[enc::sck::encode      ::U_K ].bind((*bb_scrambler)[scr::sck::scramble    ::X_N2]);
	(*LDPC_encoder)[enc::sck::encode      ::U_K ].bind((*BCH_encoder )[enc::sck::encode      ::X_N ]);
	(*itl_tx      )[itl::sck::interleave  ::nat ].bind((*LDPC_encoder)[enc::sck::encode      ::X_N ]);
	(*modem       )[mdm::sck::modulate    ::X_N1].bind((*itl_tx      )[itl::sck::interleave  ::itl ]);
	(*framer      )[frm::sck::generate    ::Y_N1].bind((*modem       )[mdm::sck::modulate    ::X_N2]);
	(*pl_scrambler)[scr::sck::scramble    ::X_N1].bind((*framer      )[frm::sck::generate    ::Y_N2]);
	(*channel     )[chn::sck::add_noise   ::X_N ].bind((*pl_scrambler)[scr::sck::scramble    ::X_N2]);
	(*pl_scrambler)[scr::sck::descramble  ::Y_N1].bind((*channel     )[chn::sck::add_noise   ::Y_N ]);
	(*framer      )[frm::sck::remove_plh  ::Y_N1].bind((*pl_scrambler)[scr::sck::descramble  ::Y_N2]);
	(*modem       )[mdm::sck::demodulate  ::Y_N1].bind((*framer      )[frm::sck::remove_plh  ::Y_N2]);
	(*itl_rx      )[itl::sck::deinterleave::itl ].bind((*modem       )[mdm::sck::demodulate  ::Y_N2]);
	(*LDPC_decoder)[dec::sck::decode_siho ::Y_N ].bind((*itl_rx      )[itl::sck::deinterleave::nat ]);
	(*BCH_decoder )[dec::sck::decode_hiho ::Y_N ].bind((*LDPC_decoder)[dec::sck::decode_siho ::V_K ]);
	(*bb_scrambler)[scr::sck::descramble  ::Y_N1].bind((*BCH_decoder )[dec::sck::decode_hiho ::V_K ]);
	(*monitor     )[mnt::sck::check_errors::U   ].bind((*source      )[src::sck::generate    ::U_K ]);
	(*monitor     )[mnt::sck::check_errors::V   ].bind((*bb_scrambler)[scr::sck::descramble  ::Y_N2]);

	// reset the memory of the decoder after the end of each communication
	monitor->add_handler_check(std::bind(&module::Decoder::reset, LDPC_decoder));

	// a loop over the various SNRs
	for (auto ebn0 = params.ebn0_min; ebn0 < params.ebn0_max; ebn0 += params.ebn0_step)
	{
		// compute the code rate
		const float R = (float)params.K_bch / (float)params.N_ldpc;

		// compute the current sigma for the channel noise
		const auto esn0  = tools::ebn0_to_esn0 (ebn0, R, params.bps);
		const auto sigma = tools::esn0_to_sigma(esn0);

#pragma omp single
{
		noise.set_noise(sigma, ebn0, esn0);

		// display the performance (BER and FER) in real time (in a separate thread)
		terminal->start_temp_report();
}
// end of #pragma omp single

		// update the sigma of the modem and the channel
		LDPC_cdc->set_noise(noise);
		modem   ->set_noise(noise);
		channel ->set_noise(noise);

		// tasks execution
		while (!monitor_red->is_done_all() && !terminal->is_interrupt())
		{
			(*source      )[src::tsk::generate    ].exec();
			(*bb_scrambler)[scr::tsk::scramble    ].exec();
			(*BCH_encoder )[enc::tsk::encode      ].exec();
			(*LDPC_encoder)[enc::tsk::encode      ].exec();
			(*itl_tx      )[itl::tsk::interleave  ].exec();
			(*modem       )[mdm::tsk::modulate    ].exec();
			(*framer      )[frm::tsk::generate    ].exec();
			(*pl_scrambler)[scr::tsk::scramble    ].exec();
			(*channel     )[chn::tsk::add_noise   ].exec();
			(*pl_scrambler)[scr::tsk::descramble  ].exec();
			(*framer      )[frm::tsk::remove_plh  ].exec();
			(*modem       )[mdm::tsk::demodulate  ].exec();
			(*itl_rx      )[itl::tsk::deinterleave].exec();
			(*LDPC_decoder)[dec::tsk::decode_siho ].exec();
			(*BCH_decoder )[dec::tsk::decode_hiho ].exec();
			(*bb_scrambler)[scr::tsk::descramble  ].exec();
			(*monitor     )[mnt::tsk::check_errors].exec();
		}

// need to wait all the threads here before to reset the 'monitors' and 'terminal' states
#pragma omp barrier

#pragma omp single
{
		// final reduction
		const bool fully = true, final = true;
		monitor_red->is_done_all(fully, final);

		// display the performance (BER and FER) in the terminal
		terminal->final_report();

		// reset the monitors and the terminal for the next SNR
		monitor_red->reset_all();
		terminal->reset();

		// display the statistics of the tasks (if enabled)
		if (params.stats)
		{
			std::vector<std::vector<const module::Module*>> modules_stats(modules[0].size());
			for (size_t m = 0; m < modules[0].size(); m++)
				for (size_t t = 0; t < modules.size(); t++)
					modules_stats[m].push_back(modules[t][m]);

			std::cout << "#" << std::endl;
			const auto ordered = true;
			tools::Stats::show(modules_stats, ordered);

			for (size_t m = 0; m < modules[0].size(); m++)
				for (size_t t = 0; t < modules.size(); t++)
					for (auto &ta : modules_stats[m][t]->tasks)
						ta->reset_stats();

			if (ebn0 + params.ebn0_step < params.ebn0_max)
			{
				std::cout << "#" << std::endl;
				terminal->legend();
			}
		}
}
// end of #pragma omp single

		// if user pressed Ctrl+c twice, exit the SNRs loop
		if (terminal->is_over()) break;
	}
}
// end of #pragma omp parallel

	std::cout << "#" << std::endl;
	std::cout << "# End of the simulation" << std::endl;

	return EXIT_SUCCESS;
}

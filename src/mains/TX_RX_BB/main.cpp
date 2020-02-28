#include <vector>
#include <thread>
#include <numeric>
#include <iostream>

#include <aff3ct.hpp>

#include "Factory/DVBS2/DVBS2.hpp"

using namespace aff3ct;
using namespace aff3ct::module;

// global parameters
constexpr bool enable_logs = true;

// aliases
namespace aff3ct { namespace module { using Monitor_BFER_reduction = Monitor_reduction<Monitor_BFER<>>; } }
template<class T> using uptr = std::unique_ptr<T>;

int main(int argc, char** argv)
{
	// get the parameter to configure the tools and modules
	auto params = factory::DVBS2(argc, argv);

	std::cout << "[trace]" << std::endl;
	std::map<std::string,tools::header_list> headers;
	std::vector<factory::Factory*> param_vec;
	param_vec.push_back(&params);
	tools::Header::print_parameters(param_vec, false, std::cout);

	// construct tools
	tools::Sigma<> noise_ref, noise_est, noise_fake(1.f);
	tools::Constellation_user<float> cstl(params.constellation_file);
	tools::BCH_polynomial_generator<> poly_gen(params.N_bch_unshortened, 12, params.bch_prim_poly);
	tools::Gaussian_noise_generator_fast<R> gen;
	uptr<tools::Interleaver_core<>> itl_core(factory::DVBS2::build_itl_core<>(params));

	// construct modules
	uptr<Source<>                   > source      (factory::DVBS2::build_source           <>(params             ));
	uptr<Scrambler<>                > bb_scrambler(factory::DVBS2::build_bb_scrambler     <>(params             ));
	uptr<Encoder<>                  > BCH_encoder (factory::DVBS2::build_bch_encoder      <>(params, poly_gen   ));
	uptr<Decoder_HIHO<>             > BCH_decoder (factory::DVBS2::build_bch_decoder      <>(params, poly_gen   ));
	uptr<tools::Codec_SIHO<>        > LDPC_cdc    (factory::DVBS2::build_ldpc_cdc         <>(params             ));
	uptr<Interleaver<>              > itl_tx      (factory::DVBS2::build_itl              <>(params, *itl_core  ));
	uptr<Interleaver<float,uint32_t>> itl_rx      (factory::DVBS2::build_itl<float,uint32_t>(params, *itl_core  ));
	uptr<Modem<>                    > modem       (factory::DVBS2::build_modem            <>(params, &cstl      ));
	uptr<Channel<>                  > channel     (factory::DVBS2::build_channel          <>(params, gen, false ));
	uptr<Framer<>                   > framer      (factory::DVBS2::build_framer           <>(params             ));
	uptr<Scrambler<float>           > pl_scrambler(factory::DVBS2::build_pl_scrambler     <>(params             ));
	uptr<Estimator<>                > estimator   (factory::DVBS2::build_estimator        <>(params, &noise_ref ));
	uptr<Monitor_BFER<>             > monitor     (factory::DVBS2::build_monitor          <>(params             ));

	auto* LDPC_encoder = &LDPC_cdc->get_encoder();
	auto* LDPC_decoder = &LDPC_cdc->get_decoder_siho();

	// set the noise
	channel  ->set_noise(noise_ref );
	estimator->set_noise(noise_est );
	modem    ->set_noise(noise_fake);

	LDPC_encoder->set_custom_name("LDPC Encoder");
	LDPC_decoder->set_custom_name("LDPC Decoder");
	BCH_encoder ->set_custom_name("BCH Encoder" );
	BCH_decoder ->set_custom_name("BCH Decoder" );

	// fulfill the list of modules
	std::vector<const Module*> modules;
	modules = { bb_scrambler.get(), BCH_encoder.get(), BCH_decoder.get(), LDPC_encoder      , LDPC_decoder      ,
	            itl_tx      .get(), itl_rx     .get(), modem      .get(), framer      .get(), pl_scrambler.get(),
	            source      .get(), monitor    .get(), channel    .get(), estimator   .get()                      };

	// configuration of the module tasks
	for (auto& m : modules) for (auto& ta : m->tasks)
	{
		ta->set_autoalloc  (true              ); // enable the automatic allocation of the data in the tasks
		ta->set_debug      (params.debug      ); // disable the debug mode
		ta->set_debug_limit(params.debug_limit); // display only the 16 first bits if the debug mode is enabled
		ta->set_stats      (params.stats      ); // enable the statistics

		if (!ta->is_debug() && !ta->is_stats())
			ta->set_fast(true);
	}

	// socket binding
	(*bb_scrambler)[scr::sck::scramble     ::X_N1].bind((*source      )[src::sck::generate     ::U_K ]);
	(*BCH_encoder )[enc::sck::encode       ::U_K ].bind((*bb_scrambler)[scr::sck::scramble     ::X_N2]);
	(*LDPC_encoder)[enc::sck::encode       ::U_K ].bind((*BCH_encoder )[enc::sck::encode       ::X_N ]);
	(*itl_tx      )[itl::sck::interleave   ::nat ].bind((*LDPC_encoder)[enc::sck::encode       ::X_N ]);
	(*modem       )[mdm::sck::modulate     ::X_N1].bind((*itl_tx      )[itl::sck::interleave   ::itl ]);
	(*framer      )[frm::sck::generate     ::Y_N1].bind((*modem       )[mdm::sck::modulate     ::X_N2]);
	(*pl_scrambler)[scr::sck::scramble     ::X_N1].bind((*framer      )[frm::sck::generate     ::Y_N2]);
	(*channel     )[chn::sck::add_noise    ::X_N ].bind((*pl_scrambler)[scr::sck::scramble     ::X_N2]);
	(*pl_scrambler)[scr::sck::descramble   ::Y_N1].bind((*channel     )[chn::sck::add_noise    ::Y_N ]);
	(*framer      )[frm::sck::remove_plh   ::Y_N1].bind((*pl_scrambler)[scr::sck::descramble   ::Y_N2]);
	(*estimator   )[est::sck::rescale      ::X_N ].bind((*framer      )[frm::sck::remove_plh   ::Y_N2]);
	(*modem       )[mdm::sck::demodulate_wg::H_N ].bind((*estimator   )[est::sck::rescale      ::H_N ]);
	(*modem       )[mdm::sck::demodulate_wg::Y_N1].bind((*estimator   )[est::sck::rescale      ::Y_N ]);
	(*itl_rx      )[itl::sck::deinterleave ::itl ].bind((*modem       )[mdm::sck::demodulate_wg::Y_N2]);
	(*LDPC_decoder)[dec::sck::decode_siho  ::Y_N ].bind((*itl_rx      )[itl::sck::deinterleave ::nat ]);
	(*BCH_decoder )[dec::sck::decode_hiho  ::Y_N ].bind((*LDPC_decoder)[dec::sck::decode_siho  ::V_K ]);
	(*bb_scrambler)[scr::sck::descramble   ::Y_N1].bind((*BCH_decoder )[dec::sck::decode_hiho  ::V_K ]);
	(*monitor     )[mnt::sck::check_errors ::U   ].bind((*source      )[src::sck::generate     ::U_K ]);
	(*monitor     )[mnt::sck::check_errors ::V   ].bind((*bb_scrambler)[scr::sck::descramble   ::Y_N2]);

	tools::Chain chain((*source)[src::tsk::generate], std::thread::hardware_concurrency());

	if (enable_logs)
	{
		std::ofstream f("chain.dot");
		chain.export_dot(f);
	}

	// registering to noise updates
	for (auto &m : chain.get_modules<Channel<>>())
		noise_ref.record_callback_update([m](){ m->notify_noise_update(); });

	// set different seeds in the modules that uses PRNG
	std::mt19937 prng;
	for (auto &m : chain.get_modules<tools::Interface_set_seed>())
		m->set_seed(prng());

	// allocate a common monitor module to reduce all the monitors
	Monitor_BFER_reduction monitor_red(chain.get_modules<Monitor_BFER<>>());
	monitor_red.set_reduce_frequency(std::chrono::milliseconds(500));
	monitor_red.check_reducible();

	// allocate reporters to display results in the terminal
	tools::Reporter_noise<>      rep_noise(noise_ref  ); // report the noise values (Es/N0 and Eb/N0)
	tools::Reporter_BFER<>       rep_BFER (monitor_red); // report the bit/frame error rates
	tools::Reporter_throughput<> rep_thr  (monitor_red); // report the simulation throughputs

	// allocate a terminal that will display the collected data from the reporters
	tools::Terminal_std terminal({ &rep_noise, &rep_BFER, &rep_thr });

	// display the legend in the terminal
	terminal.legend();

	// a loop over the various SNRs
	for (auto ebn0 = params.ebn0_min; ebn0 < params.ebn0_max; ebn0 += params.ebn0_step)
	{
		// compute the code rate
		const float R = (float)params.K_bch / (float)params.N_ldpc;

		// compute the current sigma for the channel noise
		const auto esn0  = tools::ebn0_to_esn0 (ebn0, R, params.bps);
		const auto sigma = tools::esn0_to_sigma(esn0);

		noise_ref.set_values(sigma, ebn0, esn0);

		// display the performance (BER and FER) in real time (in a separate thread)
		if (params.ter_freq != std::chrono::nanoseconds(0))
			terminal.start_temp_report(params.ter_freq);

		// execute the simulation chain
		chain.exec([&monitor_red]() { return monitor_red.is_done_all() || tools::Terminal::is_interrupt(); });

		// final reduction
		const bool fully = true;
		monitor_red.is_done_all(fully);

		// display the performance (BER and FER) in the terminal
		terminal.final_report();

		// reset the monitors and the terminal for the next SNR
		monitor_red.reset_all();
		terminal.reset();

		// display the statistics of the tasks (if enabled)
		if (params.stats)
		{
			std::cout << "#" << std::endl;
			const auto ordered = true;
			tools::Stats::show(chain.get_tasks_per_types(), ordered);

			for (auto &tt : chain.get_tasks_per_types())
				for (auto &t : tt)
					t->reset();

			if (ebn0 + params.ebn0_step < params.ebn0_max)
			{
				std::cout << "#" << std::endl;
				terminal.legend();
			}
		}

		// if user pressed Ctrl+c twice, exit the SNRs loop
		if (tools::Terminal::is_over()) break;
	}

	std::cout << "#" << std::endl;
	std::cout << "# End of the simulation" << std::endl;

	return EXIT_SUCCESS;
}

#include <vector>
#include <thread>
#include <numeric>
#include <iostream>

#include <aff3ct.hpp>
#include <streampu.hpp>

#include "Factory/DVBS2/DVBS2.hpp"

using namespace aff3ct;
using namespace aff3ct::module;

#define MULTI_THREADED // comment this line to disable multi-threaded TX/RX baseband

// global parameters
constexpr bool enable_logs = false;
#ifdef MULTI_THREADED
const size_t n_threads = std::thread::hardware_concurrency();
#else
const size_t n_threads = 1;
#endif /* MULTI_THREADED */

// aliases
namespace aff3ct { namespace module { using Monitor_BFER_reduction = tools::Monitor_reduction<Monitor_BFER<>>; } }
template<class T> using uptr = std::unique_ptr<T>;

int main(int argc, char** argv)
{
	// setup signal handlers
	spu::tools::Signal_handler::init();

	// get the parameter to configure the tools and modules
	auto params = factory::DVBS2(argc, argv);

	std::cout << "[trace]" << std::endl;
	std::map<std::string,tools::header_list> headers;
	std::vector<factory::Factory*> param_vec;
	param_vec.push_back(&params);
	tools::Header::print_parameters(param_vec, false, std::cout);

	// construct tools
	tools::Sigma<> noise_ref;
	tools::Constellation_user<float> cstl(params.constellation_file);
	tools::BCH_polynomial_generator<> poly_gen(params.N_bch_unshortened, 12, params.bch_prim_poly);
	tools::Gaussian_noise_generator_fast<R> gen;
	uptr<tools::Interleaver_core<>> itl_core(factory::DVBS2::build_itl_core<>(params));

	// construct modules
	uptr<spu::module::Source<>      > source      (factory::DVBS2::build_source           <>(params            ));
	uptr<Scrambler<>                > bb_scrambler(factory::DVBS2::build_bb_scrambler     <>(params            ));
	uptr<Encoder<>                  > BCH_encoder (factory::DVBS2::build_bch_encoder      <>(params, poly_gen  ));
	uptr<Decoder_HIHO<>             > BCH_decoder (factory::DVBS2::build_bch_decoder      <>(params, poly_gen  ));
	uptr<tools::Codec_SIHO<>        > LDPC_cdc    (factory::DVBS2::build_ldpc_cdc         <>(params            ));
	uptr<Interleaver<>              > itl_tx      (factory::DVBS2::build_itl              <>(params, *itl_core ));
	uptr<Interleaver<float,uint32_t>> itl_rx      (factory::DVBS2::build_itl<float,uint32_t>(params, *itl_core ));
	uptr<Modem<>                    > modem       (factory::DVBS2::build_modem            <>(params, &cstl     ));
	uptr<Channel<>                  > channel     (factory::DVBS2::build_channel          <>(params, gen, false));
	uptr<Framer<>                   > framer      (factory::DVBS2::build_framer           <>(params            ));
	uptr<Scrambler<float>           > pl_scrambler(factory::DVBS2::build_pl_scrambler     <>(params            ));
	uptr<Estimator<>                > estimator   (factory::DVBS2::build_estimator        <>(params, &noise_ref));
	uptr<Monitor_BFER<>             > monitor     (factory::DVBS2::build_monitor          <>(params            ));

	auto* LDPC_encoder = &LDPC_cdc->get_encoder();
	auto* LDPC_decoder = &LDPC_cdc->get_decoder_siho();

	// add custom name to some modules
	LDPC_encoder->set_custom_name("LDPC Encoder");
	LDPC_decoder->set_custom_name("LDPC Decoder");
	BCH_encoder ->set_custom_name("BCH Encoder" );
	BCH_decoder ->set_custom_name("BCH Decoder" );

	// socket binding
	std::vector<float> sigma(params.n_frames);
	(*bb_scrambler)[scr::sck::scramble    ::X_N1] = (*source      )[spu::module::src::sck::generate    ::out_data];
	(*BCH_encoder )[enc::sck::encode      ::U_K ] = (*bb_scrambler)[             scr::sck::scramble    ::X_N2    ];
	(*LDPC_encoder)[enc::sck::encode      ::U_K ] = (*BCH_encoder )[             enc::sck::encode      ::X_N     ];
	(*itl_tx      )[itl::sck::interleave  ::nat ] = (*LDPC_encoder)[             enc::sck::encode      ::X_N     ];
	(*modem       )[mdm::sck::modulate    ::X_N1] = (*itl_tx      )[             itl::sck::interleave  ::itl     ];
	(*framer      )[frm::sck::generate    ::Y_N1] = (*modem       )[             mdm::sck::modulate    ::X_N2    ];
	(*pl_scrambler)[scr::sck::scramble    ::X_N1] = (*framer      )[             frm::sck::generate    ::Y_N2    ];
	(*channel     )[chn::sck::add_noise   ::CP  ] =                                                      sigma    ;
	(*channel     )[chn::sck::add_noise   ::X_N ] = (*pl_scrambler)[             scr::sck::scramble    ::X_N2    ];
	(*pl_scrambler)[scr::sck::descramble  ::Y_N1] = (*channel     )[             chn::sck::add_noise   ::Y_N     ];
	(*framer      )[frm::sck::remove_plh  ::Y_N1] = (*pl_scrambler)[             scr::sck::descramble  ::Y_N2    ];
	(*estimator   )[est::sck::estimate    ::X_N ] = (*framer      )[             frm::sck::remove_plh  ::Y_N2    ];
	(*modem       )[mdm::sck::demodulate  ::CP  ] = (*estimator   )[             est::sck::estimate    ::SIG     ];
	(*modem       )[mdm::sck::demodulate  ::Y_N1] = (*framer      )[             frm::sck::remove_plh  ::Y_N2    ];
	(*itl_rx      )[itl::sck::deinterleave::itl ] = (*modem       )[             mdm::sck::demodulate  ::Y_N2    ];
	(*LDPC_decoder)[dec::sck::decode_siho ::Y_N ] = (*itl_rx      )[             itl::sck::deinterleave::nat     ];
	(*BCH_decoder )[dec::sck::decode_hiho ::Y_N ] = (*LDPC_decoder)[             dec::sck::decode_siho ::V_K     ];
	(*bb_scrambler)[scr::sck::descramble  ::Y_N1] = (*BCH_decoder )[             dec::sck::decode_hiho ::V_K     ];
	(*monitor     )[mnt::sck::check_errors::U   ] = (*source      )[spu::module::src::sck::generate    ::out_data];
	(*monitor     )[mnt::sck::check_errors::V   ] = (*bb_scrambler)[             scr::sck::descramble  ::Y_N2    ];

	spu::runtime::Sequence sequence_transmission((*source)[spu::module::src::tsk::generate], n_threads);

	if (enable_logs)
	{
		std::ofstream f("tx_rx_bb_sequence_transmission.dot");
		sequence_transmission.export_dot(f);
	}

	// configuration of the sequence tasks
	for (auto& type : sequence_transmission.get_tasks_per_types()) for (auto& tsk : type)
	{
		tsk->set_debug          (params.debug      ); // disable the debug mode
		tsk->set_debug_limit    (params.debug_limit); // display only the 16 first bits if the debug mode is enabled
		tsk->set_debug_precision(8                 );
		tsk->set_stats          (params.stats      ); // enable the statistics

		// enable the fast mode (= disable the useless verifs in the tasks) if there is no debug and stats modes
		if (!tsk->is_debug() && !tsk->is_stats())
			tsk->set_fast(true);
	}

	// set different seeds in the modules that uses PRNG
	std::mt19937 prng;
	for (auto &m : sequence_transmission.get_modules<spu::tools::Interface_set_seed>())
		m->set_seed(prng());

	// allocate a common monitor module to reduce all the monitors
	Monitor_BFER_reduction monitor_red(sequence_transmission.get_modules<Monitor_BFER<>>());
	monitor_red.set_reduce_frequency(std::chrono::milliseconds(500));
	monitor_red.check_reducible();

	// allocate reporters to display results in the terminal
	tools::Reporter_noise<>      rep_noise(noise_ref  ); // report the noise values (Es/N0 and Eb/N0)
	tools::Reporter_BFER<>       rep_BFER (monitor_red); // report the bit/frame error rates
	tools::Reporter_throughput<> rep_thr  (monitor_red); // report the simulation throughputs

	// allocate a terminal that will display the collected data from the reporters
	spu::tools::Terminal_std terminal({ &rep_noise, &rep_BFER, &rep_thr });

	// display the legend in the terminal
	terminal.legend();

	// a loop over the various SNRs
	for (auto ebn0 = params.ebn0_min; ebn0 < params.ebn0_max; ebn0 += params.ebn0_step)
	{
		// compute the code rate
		const float R = (float)params.K_bch / (float)params.N_ldpc;

		// compute the current sigma for the channel noise
		const auto esn0  = tools::ebn0_to_esn0 (ebn0, R, params.bps);
		std::fill(sigma.begin(), sigma.end(), tools::esn0_to_sigma(esn0));

		noise_ref.set_values(sigma[0], ebn0, esn0);

		// display the performance (BER and FER) in real time (in a separate thread)
		if (params.ter_freq != std::chrono::nanoseconds(0))
			terminal.start_temp_report(params.ter_freq);

		// execute the simulation sequence
		sequence_transmission.exec([&monitor_red]() {
			return monitor_red.is_done_all();
		});

		// final reduction
		const bool fully = true;
		monitor_red.is_done_all(fully);

		// display the performance (BER and FER) in the terminal
		terminal.final_report();

		// reset the monitors and the terminal for the next SNR
		monitor_red.reset_all();

		// display the statistics of the tasks (if enabled)
		if (params.stats)
		{
			std::cout << "#" << std::endl;
			const auto ordered = true;
			spu::tools::Stats::show(sequence_transmission.get_tasks_per_types(), ordered);

			for (auto &tt : sequence_transmission.get_tasks_per_types())
				for (auto &t : tt)
					t->reset();

			if (ebn0 + params.ebn0_step < params.ebn0_max)
			{
				std::cout << "#" << std::endl;
				terminal.legend();
			}
		}
	}

	std::cout << "#" << std::endl;
	std::cout << "# End of the simulation" << std::endl;

	return EXIT_SUCCESS;
}

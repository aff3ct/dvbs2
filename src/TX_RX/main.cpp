#include <vector>
#include <iostream>

#include <aff3ct.hpp>

#include "Scrambler/Scrambler_BB/Scrambler_BB.hpp"
#include "Params_DVBS2O/Params_DVBS2O.hpp"
#include "Factory_DVBS2O/Factory_DVBS2O.hpp"
#include "Framer/Framer.hpp"
#include "Filter/Filter_UPFIR/Filter_UPRRC/Filter_UPRRC_ccr_naive.hpp"
#include "Scrambler/Scrambler_PL/Scrambler_PL.hpp"
#include "Sink/Sink.hpp"


using namespace aff3ct;

constexpr float ebn0_min =  15.8f;
constexpr float ebn0_max = 16.f;

int main(int argc, char** argv)
{
	auto params = Params_DVBS2O(argc, argv);

	tools::BCH_polynomial_generator<B> poly_gen (params.N_BCH_unshortened, 12);
	std::unique_ptr<tools::Constellation<R>> cstl(new tools::Constellation_user<R>(params.constellation_file));

	tools::Sigma<> noise;

	std::unique_ptr<module::Source<>                   > source      (Factory_DVBS2O::build_source      <>               (params                 ));
	std::unique_ptr<module::Scrambler<>                > bb_scrambler(Factory_DVBS2O::build_bb_scrambler<>               (params                 ));
	std::unique_ptr<module::Encoder<>                  > BCH_encoder (Factory_DVBS2O::build_bch_encoder <>               (params, poly_gen       ));
	std::unique_ptr<module::Decoder_BCH_std<>          > BCH_decoder (Factory_DVBS2O::build_bch_decoder <>               (params, poly_gen       ));
	std::unique_ptr<module::Codec_LDPC<>               > LDPC_cdc    (Factory_DVBS2O::build_ldpc_cdc    <>               (params                 ));
	std::unique_ptr<tools ::Interleaver_core<>         > itl_core    (Factory_DVBS2O::build_itl_core    <>               (params                 ));
	std::unique_ptr<module::Interleaver<>              > itl_tx      (Factory_DVBS2O::build_itl         <>               (params, *itl_core      ));
	std::unique_ptr<module::Interleaver<float,uint32_t>> itl_rx      (Factory_DVBS2O::build_itl         <float,uint32_t> (params, *itl_core));
	std::unique_ptr<module::Modem<>                    > modem       (Factory_DVBS2O::build_modem       <>               (params, std::move(cstl)));
	std::unique_ptr<module::Framer<>                   > framer      (Factory_DVBS2O::build_framer      <>               (params                 ));
	std::unique_ptr<module::Scrambler<float>           > pl_scrambler(Factory_DVBS2O::build_pl_scrambler<>               (params                 ));
	std::unique_ptr<module::Monitor_BFER<B>            > monitor     (Factory_DVBS2O::build_monitor      <>              (params                 ));

	auto& LDPC_encoder    = LDPC_cdc->get_encoder();
	auto& LDPC_decoder    = LDPC_cdc->get_decoder_siho();

	// create reporters to display results in the terminal
	std::vector<tools::Reporter*> reporters =
	{
		new tools::Reporter_noise     <>(noise   ), // report the noise values (Es/N0 and Eb/N0)
		new tools::Reporter_BFER      <>(*monitor), // report the bit/frame error rates
		new tools::Reporter_throughput<>(*monitor)  // report the simulation throughputs
	};
	// convert the vector of reporter pointers into a vector of smart pointers
	std::vector<std::unique_ptr<tools::Reporter>> reporters_uptr;
	for (auto rep : reporters) reporters_uptr.push_back(std::unique_ptr<tools::Reporter>(rep));

	// create a terminal that will display the collected data from the reporters
	std::unique_ptr<tools::Terminal> terminal(new tools::Terminal_std(reporters_uptr));

	// display the legend in the terminal
	terminal->legend();

	using namespace module;
	// configuration of the module tasks
	std::vector<const module::Module*> modules = {bb_scrambler.get(), BCH_encoder.get(), BCH_decoder.get(),
	                                              LDPC_encoder.get(), LDPC_decoder.get(), itl_tx.get(), itl_rx.get(),
	                                              modem.get(), framer.get(), pl_scrambler.get(), source.get(),
	                                              monitor.get()};
	for (auto& m : modules)
		for (auto& t : m->tasks)
		{
			t->set_autoalloc  (true ); // enable the automatic allocation of the data in the tasks
			t->set_autoexec   (false); // disable the auto execution mode of the tasks
			t->set_debug      (false); // disable the debug mode
			// t->set_debug_limit(16   ); // display only the 16 first bits if the debug mode is enabled
			t->set_stats      (true ); // enable the statistics

			// enable the fast mode (= disable the useless verifs in the tasks) if there is no debug and stats modes
			t->set_fast(!t->is_debug() && !t->is_stats());
		}
	// for (auto& t : LDPC_encoder.get()->tasks)
	// 	t->set_debug(true); // disable the debug mode

	// for (auto& t : LDPC_decoder.get()->tasks)
	// 	t->set_debug(true); // disable the debug mode


	// initialization
	poly_gen.set_g(params.BCH_gen_poly);
	itl_core->init();

	// bind
	(*bb_scrambler)[scr::sck::scramble    ::X_N1].bind((*source )     [src::sck::generate    ::U_K ]);
	(*BCH_encoder) [enc::sck::encode      ::U_K ].bind((*bb_scrambler)[scr::sck::scramble    ::X_N2]);
	(*LDPC_encoder)[enc::sck::encode      ::U_K ].bind((*BCH_encoder )[enc::sck::encode      ::X_N ]);
	(*itl_tx)      [itl::sck::interleave  ::nat ].bind((*LDPC_encoder)[enc::sck::encode      ::X_N ]);
	(*modem)       [mdm::sck::modulate    ::X_N1].bind((*itl_tx      )[itl::sck::interleave  ::itl ]);
	(*framer      )[frm::sck::generate    ::Y_N1].bind((*modem       )[mdm::sck::modulate    ::X_N2]);
	(*pl_scrambler)[scr::sck::scramble    ::X_N1].bind((*framer      )[frm::sck::generate    ::Y_N2]);
	(*pl_scrambler)[scr::sck::descramble  ::Y_N1].bind((*pl_scrambler)[scr::sck::scramble    ::X_N2]);
	(*framer)      [frm::sck::remove_plh  ::Y_N1].bind((*pl_scrambler)[scr::sck::descramble  ::Y_N2]);
	(*modem)       [mdm::sck::demodulate  ::Y_N1].bind((*framer      )[frm::sck::remove_plh  ::Y_N2]);
	(*itl_rx)      [itl::sck::deinterleave::itl ].bind((*modem       )[mdm::sck::demodulate  ::Y_N2]);
	(*LDPC_decoder)[dec::sck::decode_siho ::Y_N ].bind((*itl_rx      )[itl::sck::deinterleave::nat ]);
	(*BCH_decoder) [dec::sck::decode_hiho ::Y_N ].bind((*LDPC_decoder)[dec::sck::decode_siho ::V_K ]);
	(*bb_scrambler)[scr::sck::descramble  ::Y_N1].bind((*BCH_decoder )[dec::sck::decode_hiho ::V_K ]);
	(*monitor)     [mnt::sck::check_errors::U   ].bind((*source      )[src::sck::generate    ::U_K ]);
	(*monitor)     [mnt::sck::check_errors::V   ].bind((*bb_scrambler)[scr::sck::descramble  ::Y_N2]);


	// // reset the memory of the decoder after the end of each communication
	monitor->add_handler_check(std::bind(&module::Decoder::reset, LDPC_decoder));
	// monitor->add_handler_check(std::bind(&module::Decoder::reset, BCH_decoder));



	// a loop over the various SNRs
	const float R = (float)params.K_BCH / (float)params.N_LDPC; // compute the code rate
	for (auto ebn0 = ebn0_min; ebn0 < ebn0_max; ebn0 += 1.f)
	{
		// compute the current sigma for the channel noise
		const auto esn0  = tools::ebn0_to_esn0 (ebn0, R);
		const auto sigma = tools::esn0_to_sigma(esn0   );

		noise.set_noise(sigma, ebn0, esn0);

		// update the sigma of the modem and the channel
		LDPC_cdc->set_noise(noise);
		modem   ->set_noise(noise);
		// channel->set_noise(noise);

		// display the performance (BER and FER) in real time (in a separate thread)
		terminal->start_temp_report();
		
		// exec
		while (!monitor->fe_limit_achieved() && !terminal->is_interrupt())
		{
			(*source      )[src::tsk::generate    ].exec();
			(*bb_scrambler)[scr::tsk::scramble    ].exec();
			(*BCH_encoder )[enc::tsk::encode      ].exec();
			(*LDPC_encoder)[enc::tsk::encode      ].exec();
			(*itl_tx      )[itl::tsk::interleave  ].exec();
			(*modem       )[mdm::tsk::modulate    ].exec();
			(*framer      )[frm::tsk::generate    ].exec();
			(*pl_scrambler)[scr::tsk::scramble    ].exec();
			(*pl_scrambler)[scr::tsk::descramble  ].exec();
			(*framer)      [frm::tsk::remove_plh  ].exec();
			(*modem       )[mdm::tsk::demodulate  ].exec();
			(*itl_rx      )[itl::tsk::deinterleave].exec();
			(*LDPC_decoder)[dec::tsk::decode_siho ].exec();
			(*BCH_decoder )[dec::tsk::decode_hiho ].exec();
			(*bb_scrambler)[scr::tsk::descramble  ].exec();
			(*monitor     )[mnt::tsk::check_errors].exec();
		}

		// display the performance (BER and FER) in the terminal
		terminal->final_report();

		// if user pressed Ctrl+c twice, exit the SNRs loop
		if (terminal->is_over()) break;

		// reset the monitor and the terminal for the next SNR
		monitor->reset();
		terminal->reset();
	}

	std::cout << "#" << std::endl;

	// display the statistics of the tasks (if enabled)
	auto ordered = true;
	tools::Stats::show(modules, ordered);

	std::cout << "# End of the simulation" << std::endl;








	// float sigma = 0.5;
	// int max_fe = 100;
	// unsigned max_n_frames = 1000000000;
	
	// module::Channel_AWGN_LLR<float> channel(2 * params.PL_FRAME_SIZE, 0, false, tools::Sigma<R> (sigma), 1);
	// module::Monitor_BFER<> monitor(params.K_BCH, max_fe, max_n_frames);


	return EXIT_SUCCESS;
}

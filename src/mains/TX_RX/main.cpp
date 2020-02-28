#include <vector>
#include <numeric>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>

#include <aff3ct.hpp>

#include "Factory/DVBS2/DVBS2.hpp"
#include "Tools/Reporter/Reporter_throughput_DVBS2.hpp"
#include "Tools/Reporter/Reporter_noise_DVBS2.hpp"

using namespace aff3ct;
using namespace aff3ct::module;

// global parameters
constexpr bool enable_logs = true;

// aliases
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

	// the list of the allocated modules for the simulation
	std::vector<const module::Module*> modules_each_snr;

	// construct tools
	tools::Constellation_user<float> cstl(params.constellation_file);
	tools::BCH_polynomial_generator<> poly_gen(params.N_bch_unshortened, 12, params.bch_prim_poly);
	tools::Gaussian_noise_generator_fast<R> gen(0);
	tools::Sigma<> noise_est, noise_ref, noise_fake(1.f);
	std::unique_ptr<tools::Interleaver_core<>> itl_core(factory::DVBS2::build_itl_core<>(params));

	// construct modules
	uptr<Source<>                    > source       (factory::DVBS2::build_source                   <>(params             ));
	uptr<Channel<>                   > channel      (factory::DVBS2::build_channel                  <>(params, gen        ));
	uptr<Scrambler<>                 > bb_scrambler (factory::DVBS2::build_bb_scrambler             <>(params             ));
	uptr<Encoder<>                   > BCH_encoder  (factory::DVBS2::build_bch_encoder              <>(params, poly_gen   ));
	uptr<Decoder_HIHO<>              > BCH_decoder  (factory::DVBS2::build_bch_decoder              <>(params, poly_gen   ));
	uptr<Codec_SIHO<>                > LDPC_cdc     (factory::DVBS2::build_ldpc_cdc                 <>(params             ));
	uptr<Interleaver<>               > itl_tx       (factory::DVBS2::build_itl                      <>(params, *itl_core  ));
	uptr<Interleaver<float,uint32_t> > itl_rx       (factory::DVBS2::build_itl<float,uint32_t>        (params, *itl_core  ));
	uptr<Modem<>                     > modem        (factory::DVBS2::build_modem                    <>(params, &cstl      ));
	uptr<Filter_UPRRC_ccr_naive<>    > shaping_flt  (factory::DVBS2::build_uprrc_filter             <>(params             ));
	uptr<Multiplier_sine_ccc_naive<> > freq_shift   (factory::DVBS2::build_freq_shift               <>(params             ));
	uptr<Filter_Farrow_ccr_naive<>   > chn_frac_del (factory::DVBS2::build_channel_frac_delay       <>(params             ));
	uptr<Variable_delay_cc_naive<>   > chn_int_del  (factory::DVBS2::build_channel_int_delay        <>(params             ));
	uptr<Filter_buffered_delay<float>> chn_frm_del  (factory::DVBS2::build_channel_frame_delay      <>(params             ));
	uptr<Synchronizer_frame<>        > sync_frame   (factory::DVBS2::build_synchronizer_frame       <>(params             ));
	uptr<Framer<>                    > framer       (factory::DVBS2::build_framer                   <>(params             ));
	uptr<Scrambler<float>            > pl_scrambler (factory::DVBS2::build_pl_scrambler             <>(params             ));
	uptr<Filter_buffered_delay<>     > delay        (factory::DVBS2::build_txrx_delay               <>(params             ));
	uptr<Monitor_BFER<>              > monitor      (factory::DVBS2::build_monitor                  <>(params             ));
	uptr<Filter_RRC_ccr_naive<>      > matched_flt  (factory::DVBS2::build_matched_filter           <>(params             ));
	uptr<Synchronizer_timing<>       > sync_timing  (factory::DVBS2::build_synchronizer_timing      <>(params             ));
	uptr<Multiplier_AGC_cc_naive<>   > mult_agc     (factory::DVBS2::build_agc_shift                <>(params             ));
	uptr<Estimator<>                 > estimator    (factory::DVBS2::build_estimator                <>(params, &noise_ref ));
	uptr<Multiplier_fading_DVBS2<>  > fad_mlt      (factory::DVBS2::build_fading_mult              <>(params             ));
	uptr<Synchronizer_freq_coarse<>  > sync_coarse_f(factory::DVBS2::build_synchronizer_freq_coarse <>(params             ));
	uptr<Synchronizer_freq_fine<>    > sync_fine_pf (factory::DVBS2::build_synchronizer_freq_phase  <>(params             ));
	uptr<Synchronizer_freq_fine<>    > sync_fine_lr (factory::DVBS2::build_synchronizer_lr          <>(params             ));
	uptr<Synchronizer_step_mf_cc<>   > sync_step_mf (factory::DVBS2::build_synchronizer_step_mf_cc  <>(params,
	                                                                                                   sync_coarse_f.get(),
	                                                                                                   matched_flt  .get(),
	                                                                                                   sync_timing  .get()));
	auto* LDPC_encoder = &LDPC_cdc->get_encoder();
	auto* LDPC_decoder = &LDPC_cdc->get_decoder_siho();

	// manage noise
	estimator->set_noise(noise_est );
	modem    ->set_noise(noise_fake);
	channel  ->set_noise(noise_ref );
	auto chn_ptr = channel.get();
	noise_ref.record_callback_update([chn_ptr](){ chn_ptr->notify_noise_update(); });

	delay        ->set_custom_name("TX/RX Delay");
	LDPC_encoder ->set_custom_name("LDPC Encoder");
	LDPC_decoder ->set_custom_name("LDPC Decoder");
	BCH_encoder  ->set_custom_name("BCH Encoder" );
	BCH_decoder  ->set_custom_name("BCH Decoder" );
	sync_fine_lr ->set_custom_name("L&R F Syn"   );
	sync_fine_pf ->set_custom_name("Fine P/F Syn");
	sync_timing  ->set_custom_name("Timing Syn"  );
	sync_frame   ->set_custom_name("Frame Syn"   );
	matched_flt  ->set_custom_name("Matched Flt" );
	shaping_flt  ->set_custom_name("Shaping Flt" );
	sync_coarse_f->set_custom_name("Coarse_Synch");
	sync_step_mf ->set_custom_name("MF_Synch"    );
	freq_shift   ->set_custom_name("Mult Freq."  );
	mult_agc     ->set_custom_name("Mult AGC"    );
	chn_int_del  ->set_custom_name("Chn int del ");
	chn_frac_del ->set_custom_name("Chn frac del");

	// allocate reporters to display results in the terminal
	tools::Reporter_noise_DVBS2<>      noise_reporter(noise_est, noise_ref, true);
	tools::Reporter_BFER<>             bfer_reporter (*monitor);
	tools::Reporter_throughput_DVBS2<> tpt_reporter  (*monitor);

	tools::Terminal_std terminal({ &noise_reporter, &bfer_reporter, &tpt_reporter });

	// display the legend in the terminal
	terminal.legend();

	// fulfill the list of modules
	std::vector<const module::Module*> modules;
	modules = { bb_scrambler.get(), BCH_encoder .get(), BCH_decoder .get(), LDPC_encoder      , LDPC_decoder       ,
	            itl_tx      .get(), itl_rx      .get(), modem       .get(), framer      .get(), pl_scrambler .get(),
	            monitor     .get(), freq_shift  .get(), sync_fine_lr.get(), sync_fine_pf.get(), shaping_flt  .get(),
	            chn_frac_del.get(), mult_agc    .get(), sync_frame  .get(), delay       .get(), sync_coarse_f.get(),
	            matched_flt .get(), sync_timing .get(), sync_step_mf.get(), source      .get(), channel      .get(),
	            chn_int_del .get(), estimator   .get(), fad_mlt     .get(), chn_frm_del .get()                       };

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

	// socket binding of the full transmission chain
	// delay line for BER/FER
	(*delay        )[flt::sck::filter       ::X_N1].bind((*source      )[src::sck::generate      ::U_K ]);
	// TX
	(*bb_scrambler )[scr::sck::scramble     ::X_N1].bind((*source      )[src::sck::generate      ::U_K ]);
	(*BCH_encoder  )[enc::sck::encode       ::U_K ].bind((*bb_scrambler)[scr::sck::scramble      ::X_N2]);
	(*LDPC_encoder )[enc::sck::encode       ::U_K ].bind((*BCH_encoder )[enc::sck::encode        ::X_N ]);
	(*itl_tx       )[itl::sck::interleave   ::nat ].bind((*LDPC_encoder)[enc::sck::encode        ::X_N ]);
	(*modem        )[mdm::sck::modulate     ::X_N1].bind((*itl_tx      )[itl::sck::interleave    ::itl ]);
	(*framer       )[frm::sck::generate     ::Y_N1].bind((*modem       )[mdm::sck::modulate      ::X_N2]);
	(*pl_scrambler )[scr::sck::scramble     ::X_N1].bind((*framer      )[frm::sck::generate      ::Y_N2]);
	(*shaping_flt  )[flt::sck::filter       ::X_N1].bind((*pl_scrambler)[scr::sck::scramble      ::X_N2]);
	// Channel
	(*fad_mlt      )[mlt::sck::imultiply    ::X_N ].bind((*shaping_flt )[flt::sck::filter        ::Y_N2]);
	(*chn_frm_del  )[flt::sck::filter       ::X_N1].bind((*fad_mlt     )[mlt::sck::imultiply     ::Z_N ]);
	(*chn_int_del  )[flt::sck::filter       ::X_N1].bind((*chn_frm_del )[flt::sck::filter        ::Y_N2]);
	(*chn_frac_del )[flt::sck::filter       ::X_N1].bind((*chn_int_del )[flt::sck::filter        ::Y_N2]);
	(*freq_shift   )[mlt::sck::imultiply    ::X_N ].bind((*chn_frac_del)[flt::sck::filter        ::Y_N2]);
	(*channel      )[chn::sck::add_noise    ::X_N ].bind((*freq_shift  )[mlt::sck::imultiply     ::Z_N ]);
	// RX
	(*sync_coarse_f)[sfc::sck::synchronize  ::X_N1].bind((*channel      )[chn::sck::add_noise    ::Y_N ]);
	(*matched_flt  )[flt::sck::filter       ::X_N1].bind((*sync_coarse_f)[sfc::sck::synchronize  ::Y_N2]);
	(*sync_timing  )[stm::sck::synchronize  ::X_N1].bind((*matched_flt  )[flt::sck::filter       ::Y_N2]);
	(*sync_timing  )[stm::sck::extract      ::B_N1].bind((*sync_timing  )[stm::sck::synchronize  ::B_N1]);
	(*sync_timing  )[stm::sck::extract      ::Y_N1].bind((*sync_timing  )[stm::sck::synchronize  ::Y_N1]);
	(*mult_agc     )[mlt::sck::imultiply    ::X_N ].bind((*sync_timing  )[stm::sck::extract      ::Y_N2]);
	(*sync_frame   )[sfm::sck::synchronize  ::X_N1].bind((*mult_agc     )[mlt::sck::imultiply    ::Z_N ]);
	(*pl_scrambler )[scr::sck::descramble   ::Y_N1].bind((*sync_frame   )[sfm::sck::synchronize  ::Y_N2]);
	(*sync_fine_lr )[sff::sck::synchronize  ::X_N1].bind((*pl_scrambler )[scr::sck::descramble   ::Y_N2]);
	(*sync_fine_pf )[sff::sck::synchronize  ::X_N1].bind((*sync_fine_lr )[sff::sck::synchronize  ::Y_N2]);
	(*framer       )[frm::sck::remove_plh   ::Y_N1].bind((*sync_fine_pf )[sff::sck::synchronize  ::Y_N2]);
	(*estimator    )[est::sck::rescale      ::X_N ].bind((*framer       )[frm::sck::remove_plh   ::Y_N2]);
	(*modem        )[mdm::sck::demodulate_wg::H_N ].bind((*estimator    )[est::sck::rescale      ::H_N ]);
	(*modem        )[mdm::sck::demodulate_wg::Y_N1].bind((*estimator    )[est::sck::rescale      ::Y_N ]);
	(*itl_rx       )[itl::sck::deinterleave ::itl ].bind((*modem        )[mdm::sck::demodulate_wg::Y_N2]);
	(*LDPC_decoder )[dec::sck::decode_siho  ::Y_N ].bind((*itl_rx       )[itl::sck::deinterleave ::nat ]);
	(*BCH_decoder  )[dec::sck::decode_hiho  ::Y_N ].bind((*LDPC_decoder )[dec::sck::decode_siho  ::V_K ]);
	(*bb_scrambler )[scr::sck::descramble   ::Y_N1].bind((*BCH_decoder  )[dec::sck::decode_hiho  ::V_K ]);
	(*monitor      )[mnt::sck::check_errors ::U   ].bind((*delay        )[flt::sck::filter       ::Y_N2]);
	(*monitor      )[mnt::sck::check_errors ::V   ].bind((*bb_scrambler )[scr::sck::descramble   ::Y_N2]);

	// a loop over the various SNRs
	for (auto ebn0 = params.ebn0_min; ebn0 < params.ebn0_max; ebn0 += params.ebn0_step)
	{
		// compute the code rate
		const float R = (float)params.K_bch / (float)params.N_ldpc;

		// compute the current sigma for the channel noise
		const auto esn0  = tools::ebn0_to_esn0 (ebn0, R, params.bps);
		const auto sigma = tools::esn0_to_sigma(esn0);

		noise_ref.set_values(sigma, ebn0, esn0);

		// reset synchro modules
		shaping_flt  ->reset(); chn_frac_del->reset(); chn_int_del->reset(); freq_shift ->reset();
		sync_coarse_f->reset(); matched_flt ->reset(); sync_timing->reset(); sync_frame ->reset();
		sync_fine_lr ->reset(); sync_fine_pf->reset(); delay      ->reset(); chn_frm_del->reset();

		int delay_tx_rx = params.overall_delay;
		tpt_reporter.init();
		if (!params.perfect_sync)
		{
			// ========================================================================================================
			// WAITING PHASE ==========================================================================================
			// ========================================================================================================
			(*channel     )[chn::sck::add_noise  ::Y_N ].reset();
			(*sync_step_mf)[smf::sck::synchronize::X_N1].reset();
			(*sync_frame  )[sfm::sck::synchronize::DEL ].reset();
			(*sync_step_mf)[smf::sck::synchronize::DEL ].reset();
			(*sync_step_mf)[smf::sck::synchronize::B_N1].reset();
			(*sync_step_mf)[smf::sck::synchronize::Y_N1].reset();
			(*sync_timing )[stm::sck::extract    ::B_N1].reset();
			(*sync_timing )[stm::sck::extract    ::Y_N1].reset();

			(*sync_step_mf)[smf::sck::synchronize::X_N1].bind((*channel     )[chn::sck::add_noise  ::Y_N ]);
			(*sync_step_mf)[smf::sck::synchronize::DEL ].bind((*sync_frame  )[sfm::sck::synchronize::DEL ]);
			(*sync_timing )[stm::sck::extract    ::B_N1].bind((*sync_step_mf)[smf::sck::synchronize::B_N1]);
			(*sync_timing )[stm::sck::extract    ::Y_N1].bind((*sync_step_mf)[smf::sck::synchronize::Y_N1]);

			tools::Chain chain_waiting((*source)[src::tsk::generate], (*sync_frame)[sfm::tsk::synchronize]);

			if (enable_logs && ebn0 == params.ebn0_min)
			{
				std::ofstream f("chain_waiting.dot");
				chain_waiting.export_dot(f);
			}

			int m = 0;
			chain_waiting.exec([&](const std::vector<int>& statuses)
			{
				if (statuses.back() != status_t::SKIPPED)
				{
					terminal.temp_report(std::cerr);
					m += params.n_frames;
				}
				else
				{
					delay_tx_rx++;
					if (enable_logs)
						std::clog << rang::tag::warning << "Chain aborted! (waiting phase, m = " << m << ")"
						          << std::endl;
				}

				return sync_frame->get_packet_flag();
			});

			// ========================================================================================================
			// LEARNING PHASE 1 & 2 ===================================================================================
			// ========================================================================================================
			tools::Chain chain_learning_1_2((*source)[src::tsk::generate], (*pl_scrambler)[scr::tsk::descramble]);

			if (enable_logs && ebn0 == params.ebn0_min)
			{
				std::ofstream f("chain_learning_1_2.dot");
				chain_learning_1_2.export_dot(f);
			}

			m = 0;
			int limit = 150;
			sync_coarse_f->set_PLL_coeffs(1, 1/std::sqrt(2.0), 1e-4);
			chain_learning_1_2.exec([&](const std::vector<int>& statuses)
			{
				if (statuses.back() != status_t::SKIPPED)
				{
					terminal.temp_report(std::cerr);
					m += params.n_frames;
				}
				else
				{
					delay_tx_rx++;
					if (enable_logs)
						std::clog << rang::tag::warning << "Chain aborted! (learning phase 1&2, m = " << m << ")"
						          << std::endl;
				}

				if (limit == 150 && m >= 150)
				{
					limit = m + 150;
					sync_coarse_f->set_PLL_coeffs(1, 1/std::sqrt(2.0), 5e-5);
				}
				return m >= limit;
			});

			// ========================================================================================================
			// LEARNING PHASE 3 =======================================================================================
			// ========================================================================================================
			(*channel      )[chn::sck::add_noise  ::Y_N ].reset();
			(*matched_flt  )[flt::sck::filter     ::X_N1].reset();
			(*matched_flt  )[flt::sck::filter     ::Y_N2].reset();
			(*sync_coarse_f)[sfc::sck::synchronize::X_N1].reset();
			(*sync_coarse_f)[sfc::sck::synchronize::Y_N2].reset();
			(*sync_timing  )[stm::sck::synchronize::X_N1].reset();
			(*sync_timing  )[stm::sck::synchronize::Y_N1].reset();
			(*sync_timing  )[stm::sck::synchronize::B_N1].reset();
			(*sync_timing  )[stm::sck::extract    ::B_N1].reset();
			(*sync_timing  )[stm::sck::extract    ::Y_N1].reset();

			(*sync_coarse_f)[sfc::sck::synchronize::X_N1].bind((*channel      )[chn::sck::add_noise  ::Y_N ]);
			(*matched_flt  )[flt::sck::filter     ::X_N1].bind((*sync_coarse_f)[sfc::sck::synchronize::Y_N2]);
			(*sync_timing  )[stm::sck::synchronize::X_N1].bind((*matched_flt  )[flt::sck::filter     ::Y_N2]);
			(*sync_timing  )[stm::sck::extract    ::B_N1].bind((*sync_timing  )[stm::sck::synchronize::B_N1]);
			(*sync_timing  )[stm::sck::extract    ::Y_N1].bind((*sync_timing  )[stm::sck::synchronize::Y_N1]);

			tools::Chain chain_learning_3((*source)[src::tsk::generate], (*sync_fine_pf)[sff::tsk::synchronize]);

			if (enable_logs && ebn0 == params.ebn0_min)
			{
				std::ofstream f("chain_learning_3.dot");
				chain_learning_3.export_dot(f);
			}

			limit = m + 200;
			chain_learning_3.exec([&](const std::vector<int>& statuses)
			{
				if (statuses.back() != status_t::SKIPPED)
				{
					m += params.n_frames;
					terminal.temp_report(std::cerr);
				}
				else
				{
					delay_tx_rx++;
					if (enable_logs)
						std::clog << rang::tag::warning << "Chain aborted! (learning phase 3, m = " << m << ")"
						          << std::endl;
				}
				return m >= limit;
			});
		}
		monitor->reset();

		// ============================================================================================================
		// TRANSMISSION PHASE =========================================================================================
		// ============================================================================================================
		int m = 0;
		delay->set_delay(delay_tx_rx);
		sync_timing->set_act(true);

		tools::Chain chain_transmission((*source)[src::tsk::generate]);

		if (enable_logs && ebn0 == params.ebn0_min)
		{
			std::ofstream f("chain_transmission.dot");
			chain_transmission.export_dot(f);
		}

		chain_transmission.exec([&](const std::vector<int>& statuses)
		{
			if (statuses.back() != status_t::SKIPPED)
			{
				m += params.n_frames;
				terminal.temp_report(std::cerr);
			}
			else if (enable_logs)
				std::clog << rang::tag::warning << "Chain aborted! (learning phase 3, m = " << m << ")" << std::endl;

			if (m < delay_tx_rx) // first frame is delayed
				monitor->reset();

			return monitor->is_done() || tools::Terminal::is_interrupt();
		});

		// display the performance (BER and FER) in the terminal
		terminal.final_report();

		// reset the monitors and the terminal for the next SNR
		monitor->reset();
		terminal.reset();

		if (params.stats)
		{
			std::cout << "#" << std::endl;
			const auto ordered = true;
			tools::Stats::show(chain_transmission.get_tasks_per_types(), ordered);

			for (auto& m : modules)
				for (auto& ta : m->tasks)
					ta->reset();

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

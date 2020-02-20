#include <vector>
#include <numeric>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>

#include <aff3ct.hpp>

#include "Factory/DVBS2O/DVBS2O.hpp"
// #include "Tools/Reporter/Reporter_sfc_sff_DVBS2O.hpp"
// #include "Tools/Reporter/Reporter_sfm_DVBS2O.hpp"
// #include "Tools/Reporter/Reporter_stm_DVBS2O.hpp"
#include "Tools/Reporter/Reporter_throughput_DVBS2O.hpp"
#include "Tools/Reporter/Reporter_noise_DVBS2O.hpp"

using namespace aff3ct;

int main(int argc, char** argv)
{
	/*aff3ct::module::Filter_buffered_delay<float> buffer (10, 1, 1);
	buffer.set_delay(1);
	buffer.print_buffer();

	std::vector<float> x(10, 0);
	std::vector<float> y(10, 0);

	for (size_t k = 0; k < 20 ; k++)
	{
		std::cout << std::endl << "k = " << k << std::endl;
		std::iota(x.begin(), x.end(), x[9] + 1);
		buffer.filter(x.data(), y.data());
		buffer.print_buffer();

		std::cout << "x = ";
		for (auto i = 0; i < 10; i++)
			std::cout << x[i] << " ";
		std::cout << std::endl;
		std::cout << "y = ";
		for (auto i = 0; i < 10; i++)
			std::cout << y[i] << " ";
		std::cout << std::endl;
	}
	return(0);*/
	// get the parameter to configure the tools and modules
	auto params = factory::DVBS2O(argc, argv);

	std::cout << "[trace]" << std::endl;
	std::map<std::string,tools::header_list> headers;
	std::vector<factory::Factory*> param_vec;
	param_vec.push_back(&params);
	tools::Header::print_parameters(param_vec, false, std::cout);

	std::unique_ptr<tools::Terminal> terminal;
	tools::Sigma<> noise(1.0f);

	// the list of the allocated modules for the simulation
	std::vector<const module::Module*> modules;
	std::vector<const module::Module*> modules_each_snr;

	// construct tools
	std::unique_ptr<tools::Constellation<float>> cstl(new tools::Constellation_user<float>(params.constellation_file));
	std::unique_ptr<tools::Interleaver_core<>> itl_core(factory::DVBS2O::build_itl_core<>(params));
	tools::BCH_polynomial_generator<> poly_gen(params.N_bch_unshortened, 12, params.bch_prim_poly);
	std::unique_ptr<tools::Gaussian_noise_generator<R>> gen(new tools::Gaussian_noise_generator_fast<R>(0));
	tools::Sigma<> noise_estimated(1.0f);

	// construct modules
	std::unique_ptr<module::Source<>                    > source      (factory::DVBS2O::build_source              <>(params                   ));
	std::unique_ptr<module::Channel<>                   > channel     (factory::DVBS2O::build_channel             <>(params, *gen             ));
	std::unique_ptr<module::Scrambler<>                 > bb_scrambler(factory::DVBS2O::build_bb_scrambler        <>(params                   ));
	std::unique_ptr<module::Encoder<>                   > BCH_encoder (factory::DVBS2O::build_bch_encoder         <>(params, poly_gen         ));
	std::unique_ptr<module::Decoder_HIHO<>              > BCH_decoder (factory::DVBS2O::build_bch_decoder         <>(params, poly_gen         ));
	std::unique_ptr<tools ::Codec_SIHO<>                > LDPC_cdc    (factory::DVBS2O::build_ldpc_cdc            <>(params                   ));
	std::unique_ptr<module::Interleaver<>               > itl_tx      (factory::DVBS2O::build_itl                 <>(params, *itl_core        ));
	std::unique_ptr<module::Interleaver<float,uint32_t> > itl_rx      (factory::DVBS2O::build_itl<float,uint32_t>   (params, *itl_core        ));
	std::unique_ptr<module::Modem<>                     > modem       (factory::DVBS2O::build_modem               <>(params, cstl.get()       ));
	std::unique_ptr<module::Filter_UPRRC_ccr_naive<>    > shaping_flt (factory::DVBS2O::build_uprrc_filter        <>(params                   ));
	std::unique_ptr<module::Multiplier_sine_ccc_naive<> > freq_shift  (factory::DVBS2O::build_freq_shift          <>(params                   ));
	std::unique_ptr<module::Filter_Farrow_ccr_naive<>   > chn_frac_del(factory::DVBS2O::build_channel_frac_delay  <>(params                   ));
	std::unique_ptr<module::Variable_delay_cc_naive<>   > chn_int_del (factory::DVBS2O::build_channel_int_delay   <>(params                   ));
	std::unique_ptr<module::Filter_buffered_delay<float>> chn_frm_del (factory::DVBS2O::build_channel_frame_delay <>(params                   ));
	std::unique_ptr<module::Synchronizer_frame<>        > sync_frame  (factory::DVBS2O::build_synchronizer_frame  <>(params                   ));
	std::unique_ptr<module::Framer<>                    > framer      (factory::DVBS2O::build_framer              <>(params                   ));
	std::unique_ptr<module::Scrambler<float>            > pl_scrambler(factory::DVBS2O::build_pl_scrambler        <>(params                   ));
	std::unique_ptr<module::Filter_buffered_delay<>     > delay       (factory::DVBS2O::build_txrx_delay          <>(params                   ));
	std::unique_ptr<module::Monitor_BFER<>              > monitor     (factory::DVBS2O::build_monitor             <>(params                   ));
	std::unique_ptr<module::Filter_RRC_ccr_naive<>      > matched_flt (factory::DVBS2O::build_matched_filter      <>(params                   ));
	std::unique_ptr<module::Synchronizer_timing<>       > sync_timing (factory::DVBS2O::build_synchronizer_timing <>(params                   ));
	std::unique_ptr<module::Multiplier_AGC_cc_naive<>   > mult_agc    (factory::DVBS2O::build_agc_shift           <>(params                   ));
	//std::unique_ptr<module::Multiplier_AGC_cc_naive<>   > chn_agc     (factory::DVBS2O::build_channel_agc         <>(params                   ));
	std::unique_ptr<module::Estimator<>                 > estimator   (factory::DVBS2O::build_estimator           <>(params, &noise           ));

	std::unique_ptr<module::Synchronizer_freq_coarse<>> sync_coarse_f(factory::DVBS2O::build_synchronizer_freq_coarse <>(params             ));
	std::unique_ptr<module::Synchronizer_freq_fine<>  > sync_fine_pf (factory::DVBS2O::build_synchronizer_freq_phase  <>(params             ));
	std::unique_ptr<module::Synchronizer_freq_fine<>  > sync_fine_lr (factory::DVBS2O::build_synchronizer_lr          <>(params             ));
	std::unique_ptr<module::Synchronizer_step_mf_cc<> > sync_step_mf (factory::DVBS2O::build_synchronizer_step_mf_cc  <>(params,
	                                                                                                                       sync_coarse_f.get(),
	                                                                                                                       matched_flt  .get(),
	                                                                                                                       sync_timing  .get()));

	auto* LDPC_encoder = &LDPC_cdc->get_encoder();
	auto* LDPC_decoder = &LDPC_cdc->get_decoder_siho();

	// manage noise
	LDPC_cdc ->set_noise(noise_estimated);
	modem    ->set_noise(noise_estimated);
	estimator->set_noise(noise_estimated);
	channel  ->set_noise(noise);
	auto cdc_ptr = LDPC_cdc.get();
	auto mdm_ptr = modem   .get();
	auto chn_ptr = channel .get();
	noise_estimated.record_callback_update([cdc_ptr](){ cdc_ptr->notify_noise_update(); });
	noise_estimated.record_callback_update([mdm_ptr](){ mdm_ptr->notify_noise_update(); });
	noise          .record_callback_update([chn_ptr](){ chn_ptr->notify_noise_update(); });

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
	//chn_agc      ->set_custom_name("Mult Ch. AGC");
	freq_shift   ->set_custom_name("Mult Freq."  );
	mult_agc     ->set_custom_name("Mult AGC"    );
	chn_int_del  ->set_custom_name("Chn int del ");
	chn_frac_del ->set_custom_name("Chn frac del");

	// allocate reporters to display results in the terminal
	// tools::Reporter_sfm_DVBS2O<>     sfm_reporter  (*sync_frame   .get());
	// tools::Reporter_stm_DVBS2O<>     stm_reporter  (*sync_timing  .get());
	// tools::Reporter_sfc_sff_DVBS2O<> sfc_reporter  (*sync_coarse_f.get(), *sync_fine_lr .get(), *sync_fine_pf .get(), params.osf);
	tools::Reporter_noise_DVBS2O<>   noise_reporter(noise_estimated, noise, true);
	tools::Reporter_BFER        <>   bfer_reporter (*monitor);
	tools::Reporter_throughput_DVBS2O<> tpt_monitor(*monitor);

	// std::unique_ptr<module::Probe<> > stm_probe(stm_reporter.build_probe());
	// std::unique_ptr<module::Probe<> > sfm_probe(sfm_reporter.build_probe());
	// std::unique_ptr<module::Probe<> > spf_probe(sfc_reporter.build_spf_probe());
	// std::unique_ptr<module::Probe<> > sff_probe(sfc_reporter.build_sff_probe());
	// std::unique_ptr<module::Probe<> > sfc_probe(sfc_reporter.build_sfc_probe());

	//reporters.push_back(std::unique_ptr<tools::Reporter>(std::move(sfm_reporter))); //
	//reporters.push_back(std::unique_ptr<tools::Reporter>(std::move(stm_reporter))); //
	//reporters.push_back(std::unique_ptr<tools::Reporter>(std::move(sfc_reporter))); //

	// allocate reporters to display results in the terminal
	//reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_noise_DVBS2O<> (noise_estimated, noise, true))); // report the noise values (Es/N0 and Eb/N0)
	//reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_BFER        <> (*monitor))); //report the bit/frame error rates
	// reporters.push_back(std::unique_ptr<tools::Reporter>(&tpt_monitor)); // report the simulation throughputs

	//std::vector<tools::Reporter *> reporters = {&sfm_reporter, &stm_reporter, &sfc_reporter, &noise_reporter, &bfer_reporter, &tpt_monitor};

	// allocate a terminal that will display the collected data from the reporters
	terminal = std::unique_ptr<tools::Terminal>(new tools::Terminal_std( {/*&sfm_reporter, &stm_reporter, &sfc_reporter, */&noise_reporter, &bfer_reporter, &tpt_monitor}));


	// display the legend in the terminal
	terminal->legend();

	// fulfill the list of modules
	modules = { bb_scrambler.get(), BCH_encoder .get(), BCH_decoder  .get(), LDPC_encoder        ,
	            LDPC_decoder      , itl_tx      .get(), itl_rx       .get(), modem         .get(),
	            framer      .get(), pl_scrambler.get(), monitor      .get(), freq_shift    .get(),
	            sync_fine_lr.get(), sync_fine_pf.get(), shaping_flt  .get(), chn_frac_del  .get(),
	            mult_agc    .get(), sync_frame  .get(), delay        .get(), sync_coarse_f .get(),
	            matched_flt .get(), sync_timing .get(), sync_step_mf .get(), source        .get(),
	            channel     .get(), chn_int_del .get(), estimator   .get(), //chn_agc     .get(),
	            /*
	            stm_probe   .get(), sfm_probe   .get(), sff_probe    .get(), sfc_probe     .get(),
	            spf_probe   .get(), */ chn_frm_del .get()};

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

	using namespace module;

	// socket binding
	// TX
	(*BCH_encoder )[enc::sck::encode       ::U_K ].bind((*bb_scrambler)[scr::sck::scramble     ::X_N2]);
	(*LDPC_encoder)[enc::sck::encode       ::U_K ].bind((*BCH_encoder )[enc::sck::encode       ::X_N ]);
	(*itl_tx      )[itl::sck::interleave   ::nat ].bind((*LDPC_encoder)[enc::sck::encode       ::X_N ]);
	(*modem       )[mdm::sck::modulate     ::X_N1].bind((*itl_tx      )[itl::sck::interleave   ::itl ]);
	(*framer      )[frm::sck::generate     ::Y_N1].bind((*modem       )[mdm::sck::modulate     ::X_N2]);
	(*pl_scrambler)[scr::sck::scramble     ::X_N1].bind((*framer      )[frm::sck::generate     ::Y_N2]);
	(*shaping_flt )[flt::sck::filter       ::X_N1].bind((*pl_scrambler)[scr::sck::scramble     ::X_N2]);
	(*chn_frm_del )[flt::sck::filter       ::X_N1].bind((*shaping_flt )[flt::sck::filter       ::Y_N2]);
	(*chn_int_del )[flt::sck::filter       ::X_N1].bind((*chn_frm_del )[flt::sck::filter       ::Y_N2]);
	(*chn_frac_del)[flt::sck::filter       ::X_N1].bind((*chn_int_del )[flt::sck::filter       ::Y_N2]);
	//(*chn_agc     )[mlt::sck::imultiply    ::X_N ].bind((*chn_frac_del)[flt::sck::filter       ::Y_N2]);
	//(*freq_shift  )[mlt::sck::imultiply    ::X_N ].bind((*chn_agc     )[mlt::sck::imultiply    ::Z_N ]);
	(*freq_shift  )[mlt::sck::imultiply    ::X_N ].bind((*chn_frac_del)[flt::sck::filter       ::Y_N2]);
	(*sync_fine_lr)[sff::sck::synchronize  ::X_N1].bind((*pl_scrambler)[scr::sck::descramble   ::Y_N2]);
	(*sync_fine_pf)[sff::sck::synchronize  ::X_N1].bind((*sync_fine_lr)[sff::sck::synchronize  ::Y_N2]);
	(*framer      )[frm::sck::remove_plh   ::Y_N1].bind((*sync_fine_pf)[sff::sck::synchronize  ::Y_N2]);
	(*estimator   )[est::sck::estimate     ::X_N ].bind((*framer      )[frm::sck::remove_plh   ::Y_N2]);
	(*modem       )[mdm::sck::demodulate_wg::H_N ].bind((*estimator   )[est::sck::estimate     ::H_N ]);
	(*modem       )[mdm::sck::demodulate_wg::Y_N1].bind((*framer      )[frm::sck::remove_plh   ::Y_N2]);
	(*itl_rx      )[itl::sck::deinterleave ::itl ].bind((*modem       )[mdm::sck::demodulate_wg::Y_N2]);
	(*LDPC_decoder)[dec::sck::decode_siho  ::Y_N ].bind((*itl_rx      )[itl::sck::deinterleave ::nat ]);
	(*BCH_decoder )[dec::sck::decode_hiho  ::Y_N ].bind((*LDPC_decoder)[dec::sck::decode_siho  ::V_K ]);
	(*bb_scrambler)[scr::sck::descramble   ::Y_N1].bind((*BCH_decoder )[dec::sck::decode_hiho  ::V_K ]);
	(*monitor     )[mnt::sck::check_errors ::U   ].bind((*delay       )[flt::sck::filter       ::Y_N2]);
	(*monitor     )[mnt::sck::check_errors ::V   ].bind((*bb_scrambler)[scr::sck::descramble   ::Y_N2]);

	// reset the memory of the decoder after the end of each communication
	monitor->record_callback_check([LDPC_decoder]{LDPC_decoder->reset();});

	// a loop over the various SNRs
	for (auto ebn0 = params.ebn0_min; ebn0 < params.ebn0_max; ebn0 += params.ebn0_step)
	{
		(*bb_scrambler)[scr::sck::scramble   ::X_N1 ].bind((*source       )[src::sck::generate   ::U_K  ]);
		(*channel     )[chn::sck::add_noise  ::X_N  ].bind((*freq_shift   )[mlt::sck::imultiply  ::Z_N  ]);
		(*sync_step_mf)[smf::sck::synchronize::X_N1 ].bind((*channel      )[chn::sck::add_noise  ::Y_N  ]);
		(*sync_step_mf)[smf::sck::synchronize::delay].bind((*sync_frame   )[sfm::sck::synchronize::delay]);
		(*sync_timing )[stm::sck::extract    ::B_N1 ].bind((*sync_step_mf )[smf::sck::synchronize::B_N1 ]);
		(*sync_timing )[stm::sck::extract    ::Y_N1 ].bind((*sync_step_mf )[smf::sck::synchronize::Y_N1 ]);
		(*mult_agc    )[mlt::sck::imultiply  ::X_N  ].bind((*sync_timing  )[stm::sck::extract    ::Y_N2 ]);
		(*sync_frame  )[sfm::sck::synchronize::X_N1 ].bind((*mult_agc     )[mlt::sck::imultiply  ::Z_N  ]);
		(*pl_scrambler)[scr::sck::descramble ::Y_N1 ].bind((*sync_frame   )[sfm::sck::synchronize::Y_N2 ]);
		(*delay       )[flt::sck::filter     ::X_N1 ].bind((*source       )[src::sck::generate   ::U_K  ]);

		// const int high_priority = 0;
		// (*spf_probe)[prb::sck::probe::X_N].bind((*sync_fine_pf)[sff::sck::synchronize::Y_N2], high_priority);
		// (*sff_probe)[prb::sck::probe::X_N].bind((*sync_fine_lr)[sff::sck::synchronize::Y_N2], high_priority);
		// (*sfc_probe)[prb::sck::probe::X_N].bind((*sync_step_mf)[smf::sck::synchronize::Y_N1], high_priority);
		// (*stm_probe)[prb::sck::probe::X_N].bind((*sync_step_mf)[smf::sck::synchronize::Y_N1], high_priority);
		// (*sfm_probe)[prb::sck::probe::X_N].bind((*sync_frame  )[sfm::sck::synchronize::Y_N2], high_priority);

		// compute the code rate
		const float R = (float)params.K_bch / (float)params.N_ldpc;

		// compute the current sigma for the channel noise
		const auto esn0  = tools::ebn0_to_esn0 (ebn0, R, params.bps);
		const auto sigma = tools::esn0_to_sigma(esn0);

		noise.set_values(sigma, ebn0, esn0);

		shaping_flt  ->reset();
		chn_frac_del ->reset();
		chn_int_del  ->reset();
		freq_shift   ->reset();
		sync_coarse_f->reset();
		matched_flt  ->reset();
		sync_timing  ->reset();
		sync_frame   ->reset();
		sync_fine_lr ->reset();
		sync_fine_pf ->reset();
		delay        ->reset();
		chn_frm_del  ->reset();

		sync_coarse_f->set_PLL_coeffs(1, 1/std::sqrt(2.0), 1e-4);

		int  delay_tx_rx = params.overall_delay;
		tpt_monitor.init();
		if (!params.perfect_sync)
		{
			auto n_phase = 1;

			while (!sync_frame->get_packet_flag())
			{
				try
				{
					(*source        )[src::tsk::generate   ].exec();
					(*delay         )[flt::tsk::filter     ].exec();
					(*bb_scrambler  )[scr::tsk::scramble   ].exec();
					(*BCH_encoder   )[enc::tsk::encode     ].exec();
					(*LDPC_encoder  )[enc::tsk::encode     ].exec();
					(*itl_tx        )[itl::tsk::interleave ].exec();
					(*modem         )[mdm::tsk::modulate   ].exec();
					(*framer        )[frm::tsk::generate   ].exec();
					(*pl_scrambler  )[scr::tsk::scramble   ].exec();
					(*shaping_flt   )[flt::tsk::filter     ].exec();
					(*chn_frm_del   )[flt::tsk::filter     ].exec();
					(*chn_int_del   )[flt::tsk::filter     ].exec();
					(*chn_frac_del  )[flt::tsk::filter     ].exec();
					//(*chn_agc       )[mlt::tsk::imultiply  ].exec();
					(*freq_shift    )[mlt::tsk::imultiply  ].exec();
					(*channel       )[chn::tsk::add_noise  ].exec();
					(*sync_step_mf )[smf::tsk::synchronize ].exec();
					// (*stm_probe    )[prb::tsk::probe       ].exec();
					// (*sfc_probe    )[prb::tsk::probe       ].exec();
					(*sync_timing  )[stm::tsk::extract     ].exec(); // can raise the 'tools::processing_aborted' exception
					(*mult_agc     )[mlt::tsk::imultiply   ].exec();
					(*sync_frame   )[sfm::tsk::synchronize ].exec();
					// (*sfm_probe    )[prb::tsk::probe       ].exec();
					terminal->temp_report(std::cerr);
				}
				catch (tools::processing_aborted const& e){delay_tx_rx++;}
			}


			for (int m = 0; m < 500; m += params.n_frames)
			{
				try
				{
					(*source        )[src::tsk::generate   ].exec();
					(*delay         )[flt::tsk::filter     ].exec();
					(*bb_scrambler  )[scr::tsk::scramble   ].exec();
					(*BCH_encoder   )[enc::tsk::encode     ].exec();
					(*LDPC_encoder  )[enc::tsk::encode     ].exec();
					(*itl_tx        )[itl::tsk::interleave ].exec();
					(*modem         )[mdm::tsk::modulate   ].exec();
					(*framer        )[frm::tsk::generate   ].exec();
					(*pl_scrambler  )[scr::tsk::scramble   ].exec();
					(*shaping_flt   )[flt::tsk::filter     ].exec();
					(*chn_frm_del   )[flt::tsk::filter     ].exec();
					(*chn_int_del   )[flt::tsk::filter     ].exec();
					(*chn_frac_del  )[flt::tsk::filter     ].exec();
					//(*chn_agc       )[mlt::tsk::imultiply  ].exec();
					(*freq_shift    )[mlt::tsk::imultiply  ].exec();
					(*channel       )[chn::tsk::add_noise  ].exec();

					if (n_phase < 3)
					{
						(*sync_step_mf )[smf::tsk::synchronize].exec();
						// (*stm_probe    )[prb::tsk::probe      ].exec();
						// (*sfc_probe    )[prb::tsk::probe      ].exec();
						(*sync_timing  )[stm::tsk::extract    ].exec(); // can raise the 'tools::processing_aborted' exception
						(*mult_agc     )[mlt::tsk::imultiply  ].exec();
						(*sync_frame   )[sfm::tsk::synchronize].exec();
						// (*sfm_probe    )[prb::tsk::probe      ].exec();
						(*pl_scrambler )[scr::tsk::descramble ].exec();
						terminal->temp_report(std::cerr);
					}
					else // n_phase == 3
					{
						(*sync_coarse_f)[sfc::tsk::synchronize].exec();
						// (*sfc_probe    )[prb::tsk::probe      ].exec();
						(*matched_flt  )[flt::tsk::filter     ].exec();
						(*sync_timing  )[stm::tsk::synchronize].exec();
						// (*stm_probe    )[prb::tsk::probe      ].exec();
						(*sync_timing  )[stm::tsk::extract    ].exec(); // can raise the 'tools::processing_aborted' exception
						(*mult_agc     )[mlt::tsk::imultiply  ].exec();
						(*sync_frame   )[sfm::tsk::synchronize].exec();
						// (*sfm_probe    )[prb::tsk::probe      ].exec();
						(*pl_scrambler )[scr::tsk::descramble ].exec();
						(*sync_fine_lr )[sff::tsk::synchronize].exec();
						// (*sff_probe    )[prb::tsk::probe      ].exec();
						(*sync_fine_pf )[sff::tsk::synchronize].exec();
						// (*spf_probe    )[prb::tsk::probe      ].exec();
						terminal->temp_report(std::cerr);
					}

					if (m > 149 && n_phase == 1)
					{
						m = 150;
						n_phase++;
						sync_coarse_f->set_PLL_coeffs(1, 1/std::sqrt(2.0), 5e-5);
					}

					if (m > 299 && n_phase == 2)
					{
						m = 300;
						n_phase++;
						(*sync_coarse_f)[sfc::sck::synchronize::X_N1].bind((*channel      )[chn::sck::add_noise  ::Y_N ]);
						(*matched_flt  )[flt::sck::filter     ::X_N1].bind((*sync_coarse_f)[sfc::sck::synchronize::Y_N2]);
						(*sync_timing  )[stm::sck::synchronize::X_N1].bind((*matched_flt  )[flt::sck::filter     ::Y_N2]);
						(*sync_timing  )[stm::sck::extract    ::B_N1].bind((*sync_timing  )[stm::sck::synchronize::B_N1]);
						(*sync_timing  )[stm::sck::extract    ::Y_N1].bind((*sync_timing  )[stm::sck::synchronize::Y_N1]);
						(*mult_agc     )[mlt::sck::imultiply  ::X_N ].bind((*sync_timing  )[stm::sck::extract    ::Y_N2]);
						(*sync_frame   )[sfm::sck::synchronize::X_N1].bind((*mult_agc     )[mlt::sck::imultiply  ::Z_N ]);

						// (*sfc_probe)[prb::sck::probe::X_N ].bind((*sync_coarse_f)[sfc::sck::synchronize::Y_N2], high_priority);
						// (*stm_probe)[prb::sck::probe::X_N ].bind((*sync_timing  )[stm::sck::synchronize::Y_N1], high_priority);
					}
				}
				catch (tools::processing_aborted const& e){delay_tx_rx++;}
			}
		}
		else
		{
			(*sync_coarse_f)[sfc::sck::synchronize::X_N1].bind((*channel      )[chn::sck::add_noise  ::Y_N ]);
			(*matched_flt  )[flt::sck::filter     ::X_N1].bind((*sync_coarse_f)[sfc::sck::synchronize::Y_N2]);
			(*sync_timing  )[stm::sck::synchronize::X_N1].bind((*matched_flt  )[flt::sck::filter     ::Y_N2]);
			(*sync_timing  )[stm::sck::extract    ::B_N1].bind((*sync_timing  )[stm::sck::synchronize::B_N1]);
			(*sync_timing  )[stm::sck::extract    ::Y_N1].bind((*sync_timing  )[stm::sck::synchronize::Y_N1]);
			(*mult_agc     )[mlt::sck::imultiply  ::X_N ].bind((*sync_timing  )[stm::sck::extract    ::Y_N2]);
			(*sync_frame   )[sfm::sck::synchronize::X_N1].bind((*mult_agc     )[mlt::sck::imultiply  ::Z_N ]);

			// (*stm_probe)[prb::sck::probe::X_N].bind((*sync_timing)[stm::sck::synchronize::Y_N1], high_priority);
		}
		monitor->reset();

		for (auto& m : modules)
			for (auto& ta : m->tasks)
				ta->reset();

		int n_frames = 0;
		delay->set_delay(delay_tx_rx);
		sync_timing->set_act(true);

		while (!monitor->is_done() && !terminal->is_interrupt())
		{
			try
			{
				(*source       )[src::tsk::generate     ].exec();
				(*delay        )[flt::tsk::filter       ].exec();
				(*bb_scrambler )[scr::tsk::scramble     ].exec();
				(*BCH_encoder  )[enc::tsk::encode       ].exec();
				(*LDPC_encoder )[enc::tsk::encode       ].exec();
				(*itl_tx       )[itl::tsk::interleave   ].exec();
				(*modem        )[mdm::tsk::modulate     ].exec();
				(*framer       )[frm::tsk::generate     ].exec();
				(*pl_scrambler )[scr::tsk::scramble     ].exec();
				(*shaping_flt  )[flt::tsk::filter       ].exec();
				(*chn_frm_del  )[flt::tsk::filter       ].exec();
				(*chn_int_del  )[flt::tsk::filter       ].exec();
				(*chn_frac_del )[flt::tsk::filter       ].exec();
				//(*chn_agc      )[mlt::tsk::imultiply    ].exec();
				(*freq_shift   )[mlt::tsk::imultiply    ].exec();
				(*channel      )[chn::tsk::add_noise    ].exec();
				(*sync_coarse_f)[sfc::tsk::synchronize  ].exec();
				// (*sfc_probe    )[prb::tsk::probe        ].exec();
				(*matched_flt  )[flt::tsk::filter       ].exec();
				(*sync_timing  )[stm::tsk::synchronize  ].exec();
				// (*stm_probe    )[prb::tsk::probe        ].exec();
				(*sync_timing  )[stm::tsk::extract      ].exec(); // can raise the 'tools::processing_aborted' exception
				(*mult_agc     )[mlt::tsk::imultiply    ].exec();
				(*sync_frame   )[sfm::tsk::synchronize  ].exec();
				// (*sfm_probe    )[prb::tsk::probe        ].exec();
				(*pl_scrambler )[scr::tsk::descramble   ].exec();
				(*sync_fine_lr )[sff::tsk::synchronize  ].exec();
				// (*sff_probe    )[prb::tsk::probe        ].exec();
				(*sync_fine_pf )[sff::tsk::synchronize  ].exec();
				// (*spf_probe    )[prb::tsk::probe        ].exec();
				(*framer       )[frm::tsk::remove_plh   ].exec();
				(*estimator    )[est::tsk::estimate     ].exec();
				(*modem        )[mdm::tsk::demodulate_wg].exec();
				(*itl_rx       )[itl::tsk::deinterleave ].exec();
				(*LDPC_decoder )[dec::tsk::decode_siho  ].exec();
				(*BCH_decoder  )[dec::tsk::decode_hiho  ].exec();
				(*bb_scrambler )[scr::tsk::descramble   ].exec();
				(*monitor      )[mnt::tsk::check_errors ].exec();
			}
			catch (tools::processing_aborted const&) {}

			if (n_frames < delay_tx_rx) // first frame is delayed
				monitor->reset();


			terminal->temp_report(std::cerr);
			n_frames++;
		}

		// display the performance (BER and FER) in the terminal
		terminal->final_report();

		// reset the monitors and the terminal for the next SNR
		monitor ->reset();
		terminal->reset();

		if (params.stats)
		{
			std::vector<const module::Module*> modules_stats(modules.size());
			for (size_t m = 0; m < modules.size(); m++)
				modules_stats.push_back(modules[m]);

			std::cout << "#" << std::endl;
			const auto ordered = true;
			tools::Stats::show(modules_stats, ordered);

			for (auto& m : modules)
				for (auto& ta : m->tasks)
					ta->reset();

			if (ebn0 + params.ebn0_step < params.ebn0_max)
			{
				std::cout << "#" << std::endl;
				terminal->legend();
			}
		}
	}

	std::cout << "#" << std::endl;
	std::cout << "# End of the simulation" << std::endl;

	return EXIT_SUCCESS;
}

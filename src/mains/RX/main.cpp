#include <cmath>
#include <vector>
#include <iostream>

#include <aff3ct.hpp>

#include "Factory/Factory_DVBS2O/Factory_DVBS2O.hpp"
#include "Sink/Sink.hpp"

using namespace aff3ct;

int main(int argc, char** argv)
{
	const auto params = Params_DVBS2O(argc, argv);

	// buffers to load/store the data
	std::vector<float> matlab_input (2 * params.PL_FRAME_SIZE);
	std::vector<int>   matlab_output(params.K_BCH);

	// construct tools
	std::unique_ptr<tools::Constellation           <R>> cstl          (new tools::Constellation_user<R>(params.constellation_file));
	std::unique_ptr<tools::Interleaver_core        < >> itl_core      (Factory_DVBS2O::build_itl_core<>(params                   ));
	                tools::BCH_polynomial_generator<B > poly_gen      (params.N_BCH_unshortened, 12, params.bch_prim_poly         );
	                       Sink                         sink_to_matlab(params.mat2aff_file_name, params.aff2mat_file_name         );

	// construct modules
	std::unique_ptr<module::Scrambler<>                > bb_scrambler(Factory_DVBS2O::build_bb_scrambler<>(params                         ));
	std::unique_ptr<module::Scrambler<float>           > pl_scrambler(Factory_DVBS2O::build_pl_scrambler<>(params                         ));
	std::unique_ptr<module::Codec_SIHO<>               > LDPC_cdc    (Factory_DVBS2O::build_ldpc_cdc    <>(params                         ));
	std::unique_ptr<module::Decoder_HIHO<>             > BCH_decoder (Factory_DVBS2O::build_bch_decoder <>(params, poly_gen               ));
	std::unique_ptr<module::Modem<>                    > modem       (Factory_DVBS2O::build_modem       <>(params, std::move(cstl)        ));
	std::unique_ptr<module::Interleaver<float,uint32_t>> itl         (Factory_DVBS2O::build_itl         <float,uint32_t>(params, *itl_core));
	std::unique_ptr<module::Estimator<>                > estimator   (Factory_DVBS2O::build_estimator   <>(params                         ));
	std::unique_ptr<module::Framer<>                   > framer      (Factory_DVBS2O::build_framer      <>(params                         ));

	auto& LDPC_decoder = LDPC_cdc->get_decoder_siho();

	// configuration of the module tasks
	std::vector<const module::Module*> modules = {bb_scrambler.get(), pl_scrambler.get(), BCH_decoder.get(),
	                                              modem.get(), itl.get(), estimator.get(), framer.get(),
	                                              LDPC_decoder.get()};
	for (auto& m : modules)
		for (auto& t : m->tasks)
		{
			t->set_autoalloc  (true ); // enable the automatic allocation of the data in the tasks
			t->set_autoexec   (false); // disable the auto execution mode of the tasks
			t->set_debug      (false); // disable the debug mode
			t->set_debug_limit(16   ); // display only the 16 first bits if the debug mode is enabled
			t->set_stats      (true ); // enable the statistics

			// enable the fast mode (= disable the useless verifs in the tasks) if there is no debug and stats modes
			t->set_fast(!t->is_debug() && !t->is_stats());
		}

	using namespace module;

	// socket binding
	(*framer)      [frm::sck::remove_plh   ::Y_N1].bind(matlab_input);
	(*estimator)   [est::sck::estimate     ::Y_N ].bind((*framer)      [frm::sck::remove_plh   ::Y_N2]);
	(*modem)       [mdm::sck::demodulate_wg::H_N ].bind((*estimator)   [est::sck::estimate     ::H_N ]);
	(*modem)       [mdm::sck::demodulate_wg::Y_N1].bind((*framer)      [frm::sck::remove_plh   ::Y_N2]);
	(*itl)         [itl::sck::deinterleave ::itl ].bind((*modem)       [mdm::sck::demodulate_wg::Y_N2]);
	(*LDPC_decoder)[dec::sck::decode_siho  ::Y_N ].bind((*itl)         [itl::sck::deinterleave ::nat ]);
	(*BCH_decoder) [dec::sck::decode_hiho  ::Y_N ].bind((*LDPC_decoder)[dec::sck::decode_siho  ::V_K ]);
	(*bb_scrambler)[scr::sck::descramble   ::Y_N1].bind((*BCH_decoder) [dec::sck::decode_hiho  ::V_K ]);

	sink_to_matlab.pull_vector(matlab_input);

	// tasks execution
	(*framer)      [frm::tsk::remove_plh   ].exec();
	(*estimator)   [est::tsk::estimate     ].exec();
	modem->set_noise(tools::Sigma<float>(std::sqrt(estimator->get_sigma_n2()/2.f), 0, 0));
	(*modem)       [mdm::tsk::demodulate_wg].exec();
	(*itl)         [itl::tsk::deinterleave ].exec();
	(*LDPC_decoder)[dec::tsk::decode_siho  ].exec();
	(*BCH_decoder) [dec::tsk::decode_hiho  ].exec();
	(*bb_scrambler)[scr::tsk::descramble   ].exec();

	std::copy((int*)((*bb_scrambler)[scr::sck::descramble::Y_N2].get_dataptr()),
	          (int*)((*bb_scrambler)[scr::sck::descramble::Y_N2].get_dataptr()) + params.K_BCH,
	          matlab_output.data());

	sink_to_matlab.push_vector(matlab_output, false);

	return EXIT_SUCCESS;
}

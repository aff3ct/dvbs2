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

int main(int argc, char** argv)
{
	auto params = Params_DVBS2O(argc, argv);

	// buffers to store the data
	std::vector<int>   scrambler_in      (params.K_BCH);
	std::vector<float> shaping_in        ((params.PL_FRAME_SIZE + params.GRP_DELAY) * 2, 0.0f);
	std::vector<float> shaping_cut       (params.PL_FRAME_SIZE * 2 * params.OSF);

	// construct modules
	tools::BCH_polynomial_generator<B> poly_gen (params.N_BCH_unshortened, 12);
	std::unique_ptr<tools::Constellation<R>> cstl(new tools::Constellation_user<R>(params.constellation_file));

	Sink                                        sink_to_matlab(params.mat2aff_file_name, params.aff2mat_file_name);
	std::unique_ptr<module::Scrambler       <>> bb_scrambler  (Factory_DVBS2O::build_bb_scrambler<> (params                 ));
	std::unique_ptr<module::Encoder         <>> BCH_encoder   (Factory_DVBS2O::build_bch_encoder <> (params, poly_gen       ));
	std::unique_ptr<module::Codec           <>> LDPC_cdc      (Factory_DVBS2O::build_ldpc_cdc    <> (params                 ));
	std::unique_ptr<tools ::Interleaver_core<>> itl_core      (Factory_DVBS2O::build_itl_core    <> (params                 ));
	std::unique_ptr<module::Interleaver     <>> itl           (Factory_DVBS2O::build_itl         <> (params, *itl_core      ));
	std::unique_ptr<module::Modem           <>> modulator     (Factory_DVBS2O::build_modem       <> (params, std::move(cstl)));
	std::unique_ptr<module::Framer          <>> framer        (Factory_DVBS2O::build_framer      <> (params                 ));
	std::unique_ptr<module::Scrambler  <float>> pl_scrambler  (Factory_DVBS2O::build_pl_scrambler<> (params                 ));
    std::unique_ptr<module::Filter          <>> shaping_filter(Factory_DVBS2O::build_uprrc_filter<> (params                 ));
	
	auto& LDPC_encoder    = LDPC_cdc->get_encoder();

	// initialization
	poly_gen.set_g(params.BCH_gen_poly);
	itl_core->init();

	using namespace module;
	// configuration of the module tasks
	std::vector<const module::Module*> modules = {bb_scrambler.get(), BCH_encoder.get(), LDPC_encoder.get(), itl.get(),
	                                              modulator.get(), framer.get(), pl_scrambler.get(), 
	                                              shaping_filter.get()};
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

	// execution
	(*bb_scrambler)  [scr::sck::scramble::X_N1 ].bind(scrambler_in);
	(*BCH_encoder)   [enc::sck::encode::U_K    ].bind((*bb_scrambler)[scr::sck::scramble  ::X_N2]);
	(*LDPC_encoder)  [enc::sck::encode::U_K    ].bind((*BCH_encoder )[enc::sck::encode    ::X_N ]);
	(*itl)           [itl::sck::interleave::nat].bind((*LDPC_encoder)[enc::sck::encode    ::X_N ]);
	(*modulator)     [mdm::sck::modulate::X_N1 ].bind((*itl         )[itl::sck::interleave::itl ]);
	(*framer)        [frm::sck::generate::Y_N1 ].bind((*modulator   )[mdm::sck::modulate  ::X_N2]);
	(*pl_scrambler)  [scr::sck::scramble::X_N1 ].bind((*framer      )[frm::sck::generate  ::Y_N2]);
	(*shaping_filter)[flt::sck::filter  ::X_N1 ].bind(shaping_in);

	sink_to_matlab.pull_vector(scrambler_in);
	(*bb_scrambler)[scr::tsk::scramble  ].exec();
	(*BCH_encoder )[enc::tsk::encode    ].exec();
	(*LDPC_encoder)[enc::tsk::encode    ].exec();
	(*itl         )[itl::tsk::interleave].exec();
	(*modulator   )[mdm::tsk::modulate  ].exec();
	(*framer      )[frm::tsk::generate  ].exec();
	(*pl_scrambler)[scr::tsk::scramble  ].exec();

	std::copy( (float*)((*pl_scrambler)  [scr::sck::scramble::X_N2].get_dataptr()),
	          ((float*)((*pl_scrambler)  [scr::sck::scramble::X_N2].get_dataptr())) + (2 * params.PL_FRAME_SIZE), 
	          shaping_in.data());

	(*shaping_filter)[flt::tsk::filter  ].exec();

	std::copy((float*)((*shaping_filter) [flt::sck::filter  ::Y_N2].get_dataptr()) + (params.GRP_DELAY                       ) * params.OSF * 2, 
	          (float*)((*shaping_filter) [flt::sck::filter  ::Y_N2].get_dataptr()) + (params.GRP_DELAY + params.PL_FRAME_SIZE) * params.OSF * 2,
	          shaping_cut.data());
	
	sink_to_matlab.push_vector(shaping_cut , true);

	return 0;
}

#include <aff3ct.hpp>

#include "version.h"
#include "Factory/Factory_DVBS2O/Factory_DVBS2O.hpp"

using namespace aff3ct;
// using header_list = std::vector<std::pair<std::string,std::string>>;

int main(int argc, char** argv)
{
	// get the parameter to configure the tools and modules
	auto params = Params_DVBS2O(argc, argv);

	factory::header_list dvbs2o_header;
	dvbs2o_header.push_back(std::pair<std::string,std::string>("Git Hash",dvbs2o_sha1()));
	std::map<std::string,factory::header_list> headers;
	params.p_rad.get_headers(headers);
	auto max_n_chars = 0;
	for (auto &h : headers)
		factory::Header::compute_max_n_chars(h.second, max_n_chars);
	factory::Header::compute_max_n_chars(dvbs2o_header,max_n_chars);

	std::vector<factory::Factory::parameters*> param_vec;
	param_vec.push_back(&params.p_rad);
	factory::Header::print_parameters("DVBS2O", "DVBS2O", dvbs2o_header, max_n_chars);
	factory::Header::print_parameters(param_vec, true, std::cout);

	std::vector<float> shaping_in  ((params.PL_FRAME_SIZE) * 2, 0.0f);

	// construct tools
	std::unique_ptr<tools::Constellation           <R>> cstl          (new tools::Constellation_user<R>(params.constellation_file));
	std::unique_ptr<tools::Interleaver_core        < >> itl_core      (Factory_DVBS2O::build_itl_core<>(params                   ));
	                tools::BCH_polynomial_generator<B > poly_gen      (params.N_BCH_unshortened, 12, params.bch_prim_poly         );

	// construct modules
	std::unique_ptr<module::Source<>        > source        (Factory_DVBS2O::build_source <>     (params, 0              ));
	std::unique_ptr<module::Scrambler<>     > bb_scrambler  (Factory_DVBS2O::build_bb_scrambler<>(params                 ));
	std::unique_ptr<module::Encoder<>       > BCH_encoder   (Factory_DVBS2O::build_bch_encoder <>(params, poly_gen       ));
	std::unique_ptr<module::Codec<>         > LDPC_cdc      (Factory_DVBS2O::build_ldpc_cdc    <>(params                 ));
	std::unique_ptr<module::Interleaver<>   > itl           (Factory_DVBS2O::build_itl         <>(params, *itl_core      ));
	std::unique_ptr<module::Modem<>         > modem         (Factory_DVBS2O::build_modem       <>(params, std::move(cstl)));
	std::unique_ptr<module::Framer<>        > framer        (Factory_DVBS2O::build_framer      <>(params                 ));
	std::unique_ptr<module::Scrambler<float>> pl_scrambler  (Factory_DVBS2O::build_pl_scrambler<>(params                 ));
	std::unique_ptr<module::Filter<>        > shaping_filter(Factory_DVBS2O::build_uprrc_filter<>(params                 ));
	std::unique_ptr<module::Radio<>         > radio         (Factory_DVBS2O::build_radio<>       (params                 ));

	auto& LDPC_encoder = LDPC_cdc->get_encoder();

	// the list of the allocated modules for the simulation
	std::vector<const module::Module*> modules;


	modules = { radio.get(), source.get(),bb_scrambler.get(), BCH_encoder.get(),
	            LDPC_encoder.get(), itl.get(), modem.get(), framer.get(),
	            pl_scrambler.get(), shaping_filter.get()};

	// configuration of the module tasks
	for (auto& m : modules)
		for (auto& ta : m->tasks)
		{
			ta->set_autoalloc      (true        ); // enable the automatic allocation of the data in the tasks
			ta->set_autoexec       (false       ); // disable the auto execution mode of the tasks
			ta->set_debug          (params.debug); // disable the debug mode
			ta->set_debug_limit    (-1          ); // display only the 16 first bits if the debug mode is enabled
			ta->set_debug_precision(8           );
			ta->set_stats          (params.stats); // enable the statistics

			// enable the fast mode (= disable the useless verifs in the tasks) if there is no debug and stats modes
			ta->set_fast(false);
		}


	using namespace module;

	(*bb_scrambler  )[scr::sck::scramble  ::X_N1].bind((*source      )[src::sck::generate  ::U_K ]);
	(*BCH_encoder   )[enc::sck::encode    ::U_K ].bind((*bb_scrambler)[scr::sck::scramble  ::X_N2]);
	(*LDPC_encoder  )[enc::sck::encode    ::U_K ].bind((*BCH_encoder )[enc::sck::encode    ::X_N ]);
	(*itl           )[itl::sck::interleave::nat ].bind((*LDPC_encoder)[enc::sck::encode    ::X_N ]);
	(*modem         )[mdm::sck::modulate  ::X_N1].bind((*itl         )[itl::sck::interleave::itl ]);
	(*framer        )[frm::sck::generate  ::Y_N1].bind((*modem       )[mdm::sck::modulate  ::X_N2]);
	(*pl_scrambler  )[scr::sck::scramble  ::X_N1].bind((*framer      )[frm::sck::generate  ::Y_N2]);
	(*shaping_filter)[flt::sck::filter    ::X_N1].bind(shaping_in);
	(*radio         )[rad::sck::send      ::X_N1].bind((*shaping_filter)[flt::sck::filter  ::Y_N2]);

	while(1)
	{
		(*source)      [src::tsk::generate  ].exec();
		(*bb_scrambler)[scr::tsk::scramble  ].exec();
		(*BCH_encoder )[enc::tsk::encode    ].exec();
		(*LDPC_encoder)[enc::tsk::encode    ].exec();
		(*itl         )[itl::tsk::interleave].exec();
		(*modem       )[mdm::tsk::modulate  ].exec();
		(*framer      )[frm::tsk::generate  ].exec();
		(*pl_scrambler)[scr::tsk::scramble  ].exec();

		std::copy((float*)((*pl_scrambler)[scr::sck::scramble::X_N2].get_dataptr()),
		          ((float*)((*pl_scrambler)[scr::sck::scramble::X_N2].get_dataptr())) + (2 * params.PL_FRAME_SIZE),
		          shaping_in.data());

		(*shaping_filter)[flt::tsk::filter  ].exec();
		(*radio         )[rad::tsk::send    ].exec();
	}

	return EXIT_SUCCESS;
}

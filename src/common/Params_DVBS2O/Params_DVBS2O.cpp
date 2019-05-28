#include "Params_DVBS2O.hpp"

using namespace aff3ct;

Params_DVBS2O::
Params_DVBS2O(int argc, char** argv) 
{
	
	Argument_map_value arg_vals;
	get_arguments(argc, argv, arg_vals);

	// initialize max fe
	if (arg_vals.exist({"sim-noise-min","m"}))
		ebn0_min = arg_vals.to_float({"sim-noise-min","m"});
	else
		ebn0_min = 3.2f;
		// initialize max fe
	if (arg_vals.exist({"sim-noise-max","M"}))
		ebn0_max = arg_vals.to_float({"sim-noise-max","M"});
	else
		ebn0_max = 6.f;
		// initialize max fe
	if (arg_vals.exist({"sim-noise-step","s"}))
		ebn0_step = arg_vals.to_float({"sim-noise-step","s"});
	else
		ebn0_step = .1f;

	if (arg_vals.exist({"sim-stats"}))
		stats = true;
	else
		stats = false;

	// initialize max fe
	if (arg_vals.exist({"max-fe","e"}))
		MAX_FE = arg_vals.to_int({"max-fe","e"});
	else
		MAX_FE = 100;

	// initialize MODCOD
	if (arg_vals.exist({"mod-cod"}))
		MODCOD = arg_vals.at({"mod-cod"});
	else
		MODCOD = "QPSK-S_8/9";

	if (MODCOD == "QPSK-S_8/9" || MODCOD == "")
	{
		MOD = "QPSK";
		COD = "8/9";
	}
	else if (MODCOD == "QPSK-S_3/5"  )
	{
		MOD = "QPSK";
		COD = "3/5";
	}
	else if (MODCOD == "8PSK-S_3/5"  )
	{
		MOD = "8PSK";
		COD = "3/5";
		READ_ORDER = "TOP_RIGHT";		
	}
	else if (MODCOD == "8PSK-S_8/9"  )
	{
		MOD = "8PSK";
		COD = "8/9";
		READ_ORDER = "TOP_LEFT";
	}
	else if (MODCOD == "16APSK-S_8/9")
	{
		MOD = "16APSK";
		COD = "8/9";
		READ_ORDER = "TOP_LEFT";		
	}
	else
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, MODCOD + " mod-cod scheme not supported.");

	if( MOD == "QPSK"  )
	{
		BPS = 2;
		constellation_file = "../conf/4QAM_GRAY.mod";
	}
	else if ( MOD == "8PSK"  )
	{	
		BPS = 3;
		constellation_file = "../conf/8PSK.mod";
	}
	else if ( MOD == "16APSK"){
		BPS = 4;
		constellation_file = "../conf/16APSK.mod";
	}	
	else
	{
		BPS = 2;
		constellation_file = "../conf/4QAM_GRAY.mod";
	}	

	if      ( COD == "3/5"  )
	{
		K_BCH             = 9552;
		N_BCH             = 9720;
		N_BCH_unshortened = 16383;
	}
	else if ( COD == "8/9"  )
	{
		K_BCH             = 14232;
		N_BCH             = 14400;
		N_BCH_unshortened = 16383;
	}
	
	ITL_N_COLS        = BPS;
	K_LDPC            = N_BCH;
	N_XFEC_FRAME      = N_LDPC / BPS;
	N_PILOTS          = N_XFEC_FRAME / (16 * M);
	S                 = N_XFEC_FRAME / M;
	PL_FRAME_SIZE     = M * (S + 1) + (N_PILOTS * P);

}

void Params_DVBS2O::
get_arguments(int argc, char** argv, tools::Argument_map_value& arg_vals)
{
			// build argument handler
		tools::Argument_handler ah(argc, (const char**) argv);
		tools::Argument_map_info args;
		std::vector<std::string> cmd_warn, cmd_error;

		// add possible argument inputs
		auto modcod_format = tools::Text(tools::Including_set("QPSK-S_8/9", "QPSK-S_3/5", "8PSK-S_3/5", "8PSK-S_8/9", "16APSK-S_8/9"));
		args.add({"mod-cod"},            modcod_format,                                        "Modulation and coding scheme."       );
		args.add({"max-fe","e"},         tools::Integer(tools::Positive(), tools::Non_zero()), "Modulation and coding scheme."       );
		args.add({"sim-noise-min","m"},  tools::Real(),                                        "Min Eb/N0"                           );
		args.add({"sim-noise-max","M"},  tools::Real(),                                        "Max Eb/N0"                           );
		args.add({"sim-noise-step","s"}, tools::Real(),                                        "Step Eb/N0"                          );
		args.add({"sim-stats"},          tools::None(),                                        "Display stats."                      );

		// parse user arguments
		arg_vals = ah.parse_arguments(args, cmd_warn, cmd_error);

		// exit on wrong args
		if (cmd_error.size())
		{
			if (cmd_error.size()) std::cerr << std::endl;
			for (auto w = 0; w < (int)cmd_error.size(); w++)
				std::cerr << rang::tag::error << cmd_error[w] << std::endl;

			if (cmd_warn.size()) std::cerr << std::endl;
			for (auto w = 0; w < (int)cmd_warn.size(); w++)
				std::cerr << rang::tag::warning << cmd_warn[w] << std::endl;
			exit(EXIT_FAILURE);
		}
}

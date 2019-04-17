#include "DVBS2_params.hpp"

using namespace aff3ct;

DVBS2_params::
DVBS2_params(int argc, char** argv) 
{
	std::string modcod;
	Argument_map_value arg_vals;
	get_arguments(argc, argv, arg_vals);

	// initialize modcod
	if (arg_vals.exist({"mod-cod"}))
		modcod = arg_vals.at({"mod-cod"});
	else
		modcod = "QPSK-S_8/9";

	if (modcod == "QPSK-S_8/9" || modcod == "")
	{
		K_BCH             = 14232;
		N_BCH             = 14400;
		N_BCH_unshortened = 16383;
		K_LDPC            = N_BCH;
		BPS               = 2;
		N_XFEC_FRAME      = N_LDPC / BPS;
		N_PILOTS          = N_XFEC_FRAME / (16 * M);
		S                 = N_XFEC_FRAME / 90;
		PL_FRAME_SIZE     = M * (S + 1) + (N_PILOTS * P);
	}
	else if (modcod == "QPSK-S_3/5"  )
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, modcod + " mod-cod scheme not yet supported.");
	else if (modcod == "8PSK-S_3/5"  )
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, modcod + " mod-cod scheme not yet supported.");
	else if (modcod == "8PSK-S_8/9"  )
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, modcod + " mod-cod scheme not yet supported.");
	else if (modcod == "16APSK-S_8/9")
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, modcod + " mod-cod scheme not yet supported.");
	else
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, modcod + " mod-cod scheme not supported.");
}

void DVBS2_params::
get_arguments(int argc, char** argv, tools::Argument_map_value& arg_vals)
{
			// build argument handler
		tools::Argument_handler ah(argc, (const char**) argv);
		tools::Argument_map_info args;
		std::vector<std::string> cmd_warn, cmd_error;

		// add possible argument inputs
		auto modcod_format = tools::Text(tools::Including_set("QPSK-S_8/9", "QPSK-S_3/5", "8PSK-S_3/5", "8PSK-S_8/9", "16APSK-S_8/9"));
		args.add({"mod-cod"}, modcod_format, "Modulation and coding scheme.");

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
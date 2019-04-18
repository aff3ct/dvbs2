#include "DVBS2_params.hpp"

using namespace aff3ct;

DVBS2_params::
DVBS2_params(int argc, char** argv) 
{
	
	Argument_map_value arg_vals;
	get_arguments(argc, argv, arg_vals);

	// TODO : slot number = 16
	// TODO : slot size = 90 for QPSK, 60 for 8PSK, 45 for 16APSK
	// initialize MODCOD
	if (arg_vals.exist({"mod-cod"}))
		MODCOD = arg_vals.at({"mod-cod"});
	else
		MODCOD = "QPSK-S_8/9";

	if (MODCOD == "QPSK-S_8/9" || MODCOD == "")
	{
		K_BCH             = 14232;
		N_BCH             = 14400;
		N_BCH_unshortened = 16383;
		K_LDPC            = N_BCH;
		BPS               = 2;
		N_XFEC_FRAME      = N_LDPC / BPS;
		N_PILOTS          = N_XFEC_FRAME / (16 * M);
		S                 = N_XFEC_FRAME / M;
		PL_FRAME_SIZE     = M * (S + 1) + (N_PILOTS * P);
	}
	else if (MODCOD == "QPSK-S_3/5"  )
	{
		K_BCH             = 9552;
		N_BCH             = 9720;
		N_BCH_unshortened = 16383;
		K_LDPC            = N_BCH;
		BPS               = 2;
		N_XFEC_FRAME      = N_LDPC / BPS;
		N_PILOTS          = N_XFEC_FRAME / (16 * M);
		S                 = N_XFEC_FRAME / M;
		PL_FRAME_SIZE     = M * (S + 1) + (N_PILOTS * P);
	}
	else if (MODCOD == "8PSK-S_3/5"  )
	{
		ITL_N_COLS = 3;
		READ_ORDER = "TOP_RIGHT";
		K_BCH             = 9552;
		N_BCH             = 9720;
		N_BCH_unshortened = 16383;
		K_LDPC            = N_BCH;
		BPS               = 3;
		N_XFEC_FRAME      = N_LDPC / BPS;
		N_PILOTS          = N_XFEC_FRAME / (16 * M);
		S                 = N_XFEC_FRAME / M;
		PL_FRAME_SIZE     = M * (S + 1) + (N_PILOTS * P);
	}
	else if (MODCOD == "8PSK-S_8/9"  )
	{
		ITL_N_COLS = 3;
		READ_ORDER = "TOP_LEFT";
		K_BCH             = 14232;
		N_BCH             = 14400;
		N_BCH_unshortened = 16383;
		K_LDPC            = N_BCH;
		BPS               = 3;
		N_XFEC_FRAME      = N_LDPC / BPS;
		N_PILOTS          = N_XFEC_FRAME / (16 * M);
		S                 = N_XFEC_FRAME / M;
		PL_FRAME_SIZE     = M * (S + 1) + (N_PILOTS * P);
	}
	else if (MODCOD == "16APSK-S_8/9")
	{
		ITL_N_COLS = 4;
		READ_ORDER = "TOP_LEFT";
		K_BCH             = 14232;
		N_BCH             = 14400;
		N_BCH_unshortened = 16383;
		K_LDPC            = N_BCH;
		BPS               = 4;
		N_XFEC_FRAME      = N_LDPC / BPS;
		N_PILOTS          = N_XFEC_FRAME / (16 * M);
		S                 = N_XFEC_FRAME / M;
		PL_FRAME_SIZE     = M * (S + 1) + (N_PILOTS * P);
	}
	else
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, MODCOD + " mod-cod scheme not supported.");
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
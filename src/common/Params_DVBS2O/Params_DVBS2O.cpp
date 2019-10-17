#include "Factory/Module/Radio/Radio.hpp"

#include "Params_DVBS2O/Params_DVBS2O.hpp"

using namespace aff3ct;

Params_DVBS2O::
Params_DVBS2O(int argc, char** argv) 
{
	
	cli::Argument_map_value arg_vals;
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

	if (arg_vals.exist({"sim-debug","d"}))
		debug = true;
	else
		debug = false;

	if (arg_vals.exist({"sim-stats"}))
		stats = true;
	else
		stats = false;

	filtered = true;

	if (arg_vals.exist({"dec-ite"}))
		LDPC_NITE = arg_vals.to_int({"dec-ite"});
	else
		LDPC_NITE = 50;

	if (arg_vals.exist({"dec-implem"}))
		LDPC_IMPLEM = arg_vals.at({"dec-implem"});
	else
		LDPC_IMPLEM = "SPA";

	if (arg_vals.exist({"dec-simd"}))
		LDPC_SIMD = arg_vals.at({"dec-simd"});
	else
		LDPC_SIMD = "";

	// initialize max fe
	if (arg_vals.exist({"max-fe","e"}))
		MAX_FE = arg_vals.to_int({"max-fe","e"});
	else
		MAX_FE = 100;

	// initialize max fe
	if (arg_vals.exist({"chn-max-freq-shift"}))
		MAX_FREQ_SHIFT = arg_vals.to_float({"chn-max-freq-shift"});
	else
		MAX_FREQ_SHIFT = 0.0f;

	// initialize max fe
	if (arg_vals.exist({"chn-max-delay"}))
		MAX_DELAY = arg_vals.to_float({"chn-max-delay"});
	else
		MAX_DELAY = 0.0f;

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
	
	if (arg_vals.exist({"section"}))
		section = arg_vals.at({"section"});
	else
		section = "";

	ITL_N_COLS        = BPS;
	K_LDPC            = N_BCH;
	N_XFEC_FRAME      = N_LDPC / BPS;
	N_PILOTS          = N_XFEC_FRAME / (16 * M);
	S                 = N_XFEC_FRAME / M;
	PL_FRAME_SIZE     = M * (S + 1) + (N_PILOTS * P);

	p_rad.N = (this->PL_FRAME_SIZE) * 4; // 2 * N_fil
	p_rad.store(arg_vals);
}

void Params_DVBS2O::
get_arguments(int argc, char** argv, cli::Argument_map_value& arg_vals)
{
			// build argument handler
		cli::Argument_handler ah(argc, (const char**) argv);
		cli::Argument_map_info args;
		std::vector<std::string> cmd_warn, cmd_error;

		p_rad.get_description(args);

		// add possible argument inputs
		auto modcod_format = cli::Text(cli::Including_set("QPSK-S_8/9", "QPSK-S_3/5", "8PSK-S_3/5", "8PSK-S_8/9", "16APSK-S_8/9"));
		args.add({"mod-cod"},            modcod_format,                                      "Modulation and coding scheme."       );
		args.add({"chn-max-freq-shift"}, cli::Real(),                                        "Maximum Doppler shift."              );
		args.add({"chn-max-delay"},      cli::Real(),                                        "Maximum Channel Delay."              );
		args.add({"max-fe","e"},         cli::Integer(cli::Positive(), cli::Non_zero()),     "Modulation and coding scheme."       );
		args.add({"sim-noise-min","m"},  cli::Real(),                                        "Min Eb/N0"                           );
		args.add({"sim-noise-max","M"},  cli::Real(),                                        "Max Eb/N0"                           );
		args.add({"sim-noise-step","s"}, cli::Real(),                                        "Step Eb/N0"                          );
		args.add({"sim-debug", "d"},     cli::None(),                                        "Display debug."                      );
		args.add({"sim-stats"},          cli::None(),                                        "Display stats."                      );
		args.add({"dec-ite"},            cli::Integer(cli::Positive(), cli::Non_zero()),     "LDPC number of iterations"           );
		args.add({"dec-implem"},         cli::Text(cli::Including_set("SPA", "MS", "NMS")),  "LDPC Implem "                        );
		args.add({"dec-simd"},           cli::Text(cli::Including_set("INTER", "INTRA")),    "Display stats."                      );
		args.add({"section"},            cli::Text(),                                        "Section to be used in bridge binary.");

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

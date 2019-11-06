#include "Factory/Module/Radio/Radio.hpp"

#include "Tools/Display/rang_format/rang_format.h"
#include "Params_DVBS2O/Params_DVBS2O.hpp"

using namespace aff3ct;

Params_DVBS2O::
Params_DVBS2O(int argc, char** argv)
{

	cli::Argument_map_value arg_vals;
	get_arguments(argc, argv, arg_vals);

	ebn0_min       = arg_vals.exist({"sim-noise-min","m"} ) ? arg_vals.to_float({"sim-noise-min","m"} ) : 3.2f        ;
	ebn0_max       = arg_vals.exist({"sim-noise-max","M"} ) ? arg_vals.to_float({"sim-noise-max","M"} ) : 6.f         ;
	ebn0_step      = arg_vals.exist({"sim-noise-step","s"}) ? arg_vals.to_float({"sim-noise-step","s"}) : .1f         ;
	debug          = arg_vals.exist({"sim-debug","d"}     ) ? true                                      : false       ;
	stats          = arg_vals.exist({"sim-stats"}         ) ? true                                      : false       ;
	no_pll         = arg_vals.exist({"no-pll"}            ) ? true                                      : false       ;
	filtered       = true;
	LDPC_NITE      = arg_vals.exist({"dec-ite"}           ) ? arg_vals.to_int  ({"dec-ite"}           ) : 50          ;
	LDPC_IMPLEM    = arg_vals.exist({"dec-implem"}        ) ? arg_vals.at      ({"dec-implem"}        ) : "SPA"       ;
	LDPC_SIMD      = arg_vals.exist({"dec-simd"}          ) ? arg_vals.at      ({"dec-simd"}          ) : ""          ;
	MAX_FE         = arg_vals.exist({"max-fe","e"}        ) ? arg_vals.to_int  ({"max-fe","e"}        ) : 100         ;
	MAX_FREQ_SHIFT = arg_vals.exist({"chn-max-freq-shift"}) ? arg_vals.to_float({"chn-max-freq-shift"}) : 0.f         ;
	MAX_DELAY      = arg_vals.exist({"chn-max-delay"}     ) ? arg_vals.to_float({"chn-max-delay"}     ) : 0.f         ;
	MODCOD         = arg_vals.exist({"mod-cod"}           ) ? arg_vals.at      ({"mod-cod"}           ) : "QPSK-S_8/9";
	sink_path      = arg_vals.exist({"snk-path"}          ) ? arg_vals.at      ({"snk-path"}          ) : ""          ;
	section        = arg_vals.exist({"section"}           ) ? arg_vals.at      ({"section"}           ) : ""          ;
	src_type       = arg_vals.exist({"src-type"}          ) ? arg_vals.at      ({"src-type"}          ) : "RAND"      ;

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
		constellation_file = "../conf/mod/4QAM_GRAY.mod";
	}
	else if ( MOD == "8PSK"  )
	{
		BPS = 3;
		constellation_file = "../conf/mod/8PSK.mod";
	}
	else if ( MOD == "16APSK"){
		BPS = 4;
		constellation_file = "../conf/mod/16APSK.mod";
	}

	if      ( COD == "3/5"  )
	{
		K_BCH             = 9552;
		N_BCH             = 9720;
		N_BCH_unshortened = 16383;
		src_path          = "../conf/src/K_9552.src";
	}
	else if ( COD == "8/9"  )
	{
		K_BCH             = 14232;
		N_BCH             = 14400;
		N_BCH_unshortened = 16383;
		src_path          = "../conf/src/K_14232.src";
	}

	src_path       = arg_vals.exist({"src-path"}          ) ? arg_vals.at      ({"src-path"}          ) : src_path    ;

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
		auto modcod_format   = cli::Text(cli::Including_set("QPSK-S_8/9", "QPSK-S_3/5", "8PSK-S_3/5", "8PSK-S_8/9", "16APSK-S_8/9"));
		auto src_type_format = cli::Text(cli::Including_set("RAND", "USER", "USER_BIN", "AZCW"                                    ));
		args.add({"mod-cod"},            modcod_format,                                         "Modulation and coding scheme."       );
		args.add({"chn-max-freq-shift"}, cli::Real(),                                           "Maximum Doppler shift."              );
		args.add({"chn-max-delay"},      cli::Real(),                                           "Maximum Channel Delay."              );
		args.add({"max-fe","e"},         cli::Integer(cli::Positive(), cli::Non_zero()),        "Modulation and coding scheme."       );
		args.add({"sim-noise-min","m"},  cli::Real(),                                           "Min Eb/N0"                           );
		args.add({"sim-noise-max","M"},  cli::Real(),                                           "Max Eb/N0"                           );
		args.add({"sim-noise-step","s"}, cli::Real(),                                           "Step Eb/N0"                          );
		args.add({"no-pll"},             cli::None(),                                           "Disable coarse PLL."                 );
		args.add({"sim-debug", "d"},     cli::None(),                                           "Display debug."                      );
		args.add({"sim-stats"},          cli::None(),                                           "Display stats."                      );
		args.add({"src-type"},           src_type_format,                                       "Type of the binary source"           );
		args.add({"src-path"},           cli::Text(),                                           "Path of the binary source"           );
		args.add({"snk-path"},           cli::Text(),                                           "Path of the binary sink"             );
		args.add({"dec-ite"},            cli::Integer(cli::Positive(), cli::Non_zero()),        "LDPC number of iterations"           );
		args.add({"dec-implem"},         cli::Text(cli::Including_set("SPA", "MS", "NMS")),     "LDPC Implem "                        );
		args.add({"dec-simd"},           cli::Text(cli::Including_set("INTER", "INTRA")),       "Display stats."                      );
		args.add({"section"},            cli::Text(),                                           "Section to be used in bridge binary.");

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

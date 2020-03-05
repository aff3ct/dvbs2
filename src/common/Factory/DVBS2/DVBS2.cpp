#include <type_traits>

#include "DVBS2.hpp"

#include "Module/Encoder_BCH_DVBS2/Encoder_BCH_inter_DVBS2.hpp"
#include "Module/Encoder_BCH_DVBS2/Encoder_BCH_DVBS2.hpp"
#include "Module/Decoder_BCH_DVBS2/Decoder_BCH_DVBS2.hpp"

#include "Module/Synchronizer/Synchronizer_freq/Synchronizer_freq_coarse/Synchronizer_freq_coarse_DVBS2_aib.hpp"
#include "Module/Synchronizer/Synchronizer_freq/Synchronizer_freq_coarse/Synchronizer_freq_coarse_perfect.hpp"

#include "Module/Synchronizer/Synchronizer_freq/Synchronizer_freq_fine/Synchronizer_freq_phase_DVBS2_aib.hpp"
#include "Module/Synchronizer/Synchronizer_freq/Synchronizer_freq_fine/Synchronizer_Luise_Reggiannini_DVBS2_aib.hpp"
#include "Module/Synchronizer/Synchronizer_freq/Synchronizer_freq_fine/Synchronizer_freq_fine_perfect.hpp"

#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_Gardner_aib.hpp"
#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_Gardner_fast.hpp"
#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_Gardner_ultra_osf2.hpp"
#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_Gardner_fast_osf2.hpp"
#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_timing_perfect.hpp"

#include "Module/Synchronizer/Synchronizer_frame/Synchronizer_frame_DVBS2_aib.hpp"
#include "Module/Synchronizer/Synchronizer_frame/Synchronizer_frame_DVBS2_fast.hpp"
#include "Module/Synchronizer/Synchronizer_frame/Synchronizer_frame_perfect.hpp"

#include "Module/Estimator/Estimator_DVBS2.hpp"
#include "Module/Estimator/Estimator_perfect.hpp"

#include "Module/Sink/User/Sink_user_binary.hpp"

using namespace aff3ct;
using namespace aff3ct::factory;

const std::string aff3ct::factory::DVBS2_name   = "DVB-S2";
const std::string aff3ct::factory::DVBS2_prefix = "dvbs2";

DVBS2
::DVBS2(int argc, char** argv, const std::string& prefix)
: Factory(DVBS2_name, DVBS2_name, prefix)
{
	cli::Argument_handler ah(argc, (const char**) argv);
	cli::Argument_map_info args;
	std::vector<std::string> cmd_warn, cmd_error;

	get_description(args);

	// parse user arguments
	auto arg_vals = ah.parse_arguments(args, cmd_warn, cmd_error);

	store(arg_vals);

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

	if (this->display_help)
	{
		ah.print_help(args);
		exit(EXIT_FAILURE);
	}
}

DVBS2* DVBS2
::clone() const
{
	return new DVBS2(*this);
}

void DVBS2
::get_description(cli::Argument_map_info &args) const
{
	const std::string class_name = "factory::DVBS2::";

	auto modcod_format   = cli::Text(cli::Including_set("QPSK-S_8/9", "QPSK-S_3/5", "8PSK-S_3/5", "8PSK-S_8/9", "16APSK-S_8/9"                               ));
	auto src_type_format = cli::Text(cli::Including_set("RAND", "USER", "USER_BIN", "AZCW"                                                                   ));
	auto sfm_type_format = cli::Text(cli::Including_set("NORMAL", "FAST"                                                                                     ));
	args.add({"help","h"},           cli::None(),                                       "Displays help."                                                      );
	args.add({"mod-cod"},            modcod_format,                                     "Modulation and coding scheme."                                       );
	args.add({"chn-type"},           cli::Text(cli::Including_set("AWGN", "USER_ADD")), "Type of noise in the channel."                                       );
	args.add({"chn-path"},           cli::Text(),                                       "Path of the channel noise."                                          );
	args.add({"chn-max-freq-shift"}, cli::Real(),                                       "Maximum Doppler shift."                                              );
	args.add({"chn-max-delay"},      cli::Real(),                                       "Maximum Channel Delay."                                              );
	args.add({"max-fe","e"},         cli::Integer(cli::Positive(), cli::Non_zero()),    "Max number of frame errors."                                         );
	args.add({"sim-noise-min","m"},  cli::Real(),                                       "Min Eb/N0"                                                           );
	args.add({"sim-noise-max","M"},  cli::Real(),                                       "Max Eb/N0"                                                           );
	args.add({"sim-noise-step","s"}, cli::Real(),                                       "Step Eb/N0"                                                          );
	args.add({"sim-noise-ref"},      cli::Real(),                                       "Reference value for Es/N0"                                           );
	args.add({"sim-noise-path"},     cli::Text(),                                       "Path of the Es/N0 sequence"                                          );
	args.add({"no-sync-info"},       cli::None(),                                       "Disable sync information logging."                                   );
	args.add({"sim-dbg", "d"},       cli::None(),                                       "Display debug."                                                      );
	args.add({"sim-dbg-limit"},      cli::Integer(cli::Non_zero()),                     "Set max number of elts per frame to be displayed in debug mode."     );
	args.add({"sim-stats"},          cli::None(),                                       "Display stats."                                                      );
	args.add({"src-type"},           src_type_format,                                   "Type of the binary source"                                           );
	args.add({"src-path"},           cli::Text(),                                       "Path of the binary source"                                           );
	args.add({"snk-path"},           cli::Text(),                                       "Path of the binary sink"                                             );
	args.add({"dump-filename"},      cli::Text(),                                       "Name of the dump file generated by dvbs2_optique_rx_dump"            );
	args.add({"dec-ite"},            cli::Integer(cli::Positive(), cli::Non_zero()),    "LDPC number of iterations"                                           );
	args.add({"dec-implem"},         cli::Text(cli::Including_set("SPA", "MS", "NMS")), "LDPC Implem "                                                        );
	args.add({"dec-simd"},           cli::Text(cli::Including_set("INTER", "INTRA")),   "Display stats."                                                      );
	args.add({"est-type"},           cli::Text(cli::Including_set("DVBS2", "PERFECT")), "Type of estimator."                                                  );
	args.add({"section"},            cli::Text(),                                       "Section to be used in bridge binary."                                );
	args.add({"ter-freq"},           cli::Integer(cli::Positive()),                     "Terminal frequency."                                                 );
	args.add({"perfect-sync"},       cli::None(),                                       "Enable genie aided synchronization."                                 );
	args.add({"perfect-cf-sync"},    cli::None(),                                       "Enable genie aided coarse frequency synchronization."                );
	args.add({"sfm-type"},           sfm_type_format,                                   "Type of frame synchronization."                                      );
	args.add({"sfm-alpha"},          cli::Real(),                                       "Damping factor for frame synchronization."                           );
	args.add({"sfm-trigger"},        cli::Real(),                                       "Trigger value to detect signal presence."                            );
	args.add({"src-fra","f"},        cli::Integer(cli::Positive(), cli::Non_zero()),    "Inter frame level."                                                  );
	args.add({"src-fifo"},           cli::None(),                                       "Enable FIFO mode."                                                   );

	p_shp.get_description(args);
	p_stm.get_description(args);
	p_sff.get_description(args);
	p_rad.get_description(args);
}

void DVBS2
::store(const cli::Argument_map_value &vals)
{
	modcod         = vals.exist({"mod-cod"}           ) ? vals.at      ({"mod-cod"}           ) : "QPSK-S_8/9";

	modcod_init(modcod); // initialize all the parameters that are dependant on modcod
	esn0_ref                 = vals.exist({"sim-noise-ref"}      ) ? vals.to_float({"sim-noise-ref "}    ) : 0.f          ;
	esn0_seq_path            = vals.exist({"sim-noise-path"}     ) ? vals.at      ({"sim-noise-path"}    ) : ""           ;
	ebn0_min                 = vals.exist({"sim-noise-min","m"}  ) ? vals.to_float({"sim-noise-min","m"} ) : 3.2f         ;
	ebn0_max                 = vals.exist({"sim-noise-max","M"}  ) ? vals.to_float({"sim-noise-max","M"} ) : 6.f          ;
	ebn0_step                = vals.exist({"sim-noise-step","s"} ) ? vals.to_float({"sim-noise-step","s"}) : .1f          ;
	channel_type             = vals.exist({"chn-type"}           ) ? vals.at      ({"chn-type"}          ) : "AWGN"       ;
	channel_path             = vals.exist({"chn-path"}           ) ? vals.at      ({"chn-path"}          ) : channel_path ;
	max_freq_shift           = vals.exist({"chn-max-freq-shift"} ) ? vals.to_float({"chn-max-freq-shift"}) : 0.f          ;
	max_delay                = vals.exist({"chn-max-delay"}      ) ? vals.to_float({"chn-max-delay"}     ) : 2.f          ;
	if (max_delay < 2)
	{
		std::stringstream message;
		message << "Argument 'max_delay' has to be greater than 2.";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}
	ldpc_nite                = vals.exist({"dec-ite"}            ) ? vals.to_int  ({"dec-ite"}           ) : 50          ;
	max_fe                   = vals.exist({"max-fe","e"}         ) ? vals.to_int  ({"max-fe","e"}        ) : 100         ;
	sink_path                = vals.exist({"snk-path"}           ) ? vals.at      ({"snk-path"}          ) : "sink.out"  ;
	ldpc_implem              = vals.exist({"dec-implem"}         ) ? vals.at      ({"dec-implem"}        ) : "SPA"       ;
	ldpc_simd                = vals.exist({"dec-simd"}           ) ? vals.at      ({"dec-simd"}          ) : ""          ;
	est_type                 = vals.exist({"est-type"}           ) ? vals.at      ({"est-type"}          ) : "DVBS2"    ;
	section                  = vals.exist({"section"}            ) ? vals.at      ({"section"}           ) : ""          ;
	src_type                 = vals.exist({"src-type"}           ) ? vals.at      ({"src-type"}          ) : "RAND"      ;
	src_path                 = vals.exist({"src-path"}           ) ? vals.at      ({"src-path"}          ) : src_path    ;
	src_fifo                 = vals.exist({"src-fifo"}           ) ? true                                  : false       ;
	dump_filename            = vals.exist({"dump-filename"}      ) ? vals.at      ({"dump-filename"}     ) : "dump"      ;
	debug                    = vals.exist({"sim-dbg","d"}        ) ? true                                  : false       ;
	debug_limit              = vals.exist({"sim-dbg-limit"}      ) ? vals.to_int  ({"sim-dbg-limit"}     ) : -1          ;
	stats                    = vals.exist({"sim-stats"}          ) ? true                                  : false       ;
	no_sync_info             = vals.exist({"no-sync-info"}       ) ? true                                  : false       ;
	sfm_type                 = vals.exist({"sfm-type"}           ) ? vals.at      ({"sfm-type"}          ) : "NORMAL"    ;
	perfect_sync             = vals.exist({"perfect-sync"}       ) ? true                                  : false       ;
	perfect_coarse_freq_sync = vals.exist({"perfect-cf-sync"}    ) ? true                                  : false       ;
	n_frames                 = vals.exist({"src-fra","f"}        ) ? vals.to_int  ({"src-fra","f"}       ) : 1           ;
	sfm_alpha                = vals.exist({"sfm-alpha"}          ) ? vals.to_float({"sfm-alpha"}         ) : 0.9f        ;
	sfm_trigger              = vals.exist({"sfm-trigg"}          ) ? vals.to_float({"sfm-trigger"}       ) : 25.0f       ;

	display_help = false;
	if(vals.exist({"help","h"}))
		display_help = true;

	if (vals.exist({"sim-dbg-limit"}))
		debug = true;

	if (perfect_sync)
	{
		perfect_coarse_freq_sync = true;
		//stm_type                 = "PERFECT";
		//perfect_pf_freq_sync     = true;
		//perfect_lr_freq_sync     = true;
	}

	if (vals.exist({"ter-freq"}))
		ter_freq = std::chrono::milliseconds(vals.to_int  ({"ter-freq"}));
	else
		ter_freq = std::chrono::milliseconds(500L);


	p_shp.N_symbols = this->pl_frame_size;
	p_shp.n_frames  = n_frames;
	p_shp.store(vals);

	p_rad.N = (this->pl_frame_size) * p_shp.osf; // 2 * N_fil
	p_rad.n_frames = n_frames;
	p_rad.store(vals);

	p_sff.N        = this->pl_frame_size; // 2 * N_fil
	p_sff.n_frames = n_frames;
	p_sff.store(vals);

	p_stm.N         = p_shp.osf*this->pl_frame_size; // 2 * N_fil
	p_stm.osf       = p_shp.osf;
	p_stm.n_frames  = n_frames;
	p_stm.ref_delay = max_delay;
	p_stm.store(vals);

	int int_delay = (int)max_delay - 2;
	int N_cplx = pl_frame_size * p_shp.osf;
	overall_delay = int_delay/ N_cplx + 1;
}

std::vector<std::string> DVBS2
::get_names() const
{
	std::vector<std::string> n;
	n.push_back(this->get_name());
	n.push_back(this->p_shp.get_name());
	n.push_back(this->p_stm.get_name());
	n.push_back(this->p_sff.get_name());
	n.push_back(this->p_rad.get_name());
	return n;
}

std::vector<std::string> DVBS2
::get_short_names() const
{
	std::vector<std::string> sn;
	sn.push_back(this->get_short_name());
	sn.push_back(this->p_shp.get_short_name());
	sn.push_back(this->p_stm.get_short_name());
	sn.push_back(this->p_sff.get_short_name());
	sn.push_back(this->p_rad.get_short_name());
	return sn;
}

std::vector<std::string> DVBS2
::get_prefixes() const
{
	std::vector<std::string> p;
	p.push_back(this->get_prefix());
	p.push_back(this->p_shp.get_prefix());
	p.push_back(this->p_stm.get_prefix());
	p.push_back(this->p_sff.get_prefix());
	p.push_back(this->p_rad.get_prefix());
	return p;
}

void DVBS2
::get_headers(std::map<std::string,tools::header_list>& headers, const bool full) const
{
	auto p = this->get_prefix();

	headers[p].push_back(std::make_pair("Modulation and coding", this->modcod                            ));
	headers[p].push_back(std::make_pair("Min  Eb/N0"           , std::to_string(this->ebn0_min)          ));
	headers[p].push_back(std::make_pair("Max  Eb/N0"           , std::to_string(this->ebn0_max)          ));
	headers[p].push_back(std::make_pair("Step Eb/N0"           , std::to_string(this->ebn0_step)         ));
	headers[p].push_back(std::make_pair("Max frame errors"     , std::to_string(this->max_fe)            ));
	headers[p].push_back(std::make_pair("Type of channel"      , this->channel_type                      ));
	if (this->max_freq_shift != 0)
		headers[p].push_back(std::make_pair("Maximum Doppler shift", std::to_string(this->max_freq_shift)));
	if (this->max_delay != 0)
		headers[p].push_back(std::make_pair("Maximum Channel Delay", std::to_string(this->max_delay)));
	headers[p].push_back(std::make_pair("LDPC implem"          , this->ldpc_implem                   ));
	headers[p].push_back(std::make_pair("LDPC n iterations"    , std::to_string(this->ldpc_nite)     ));
	if (this->src_path != "")
		headers[p].push_back(std::make_pair("LDPC simd"            , this->ldpc_simd                     ));
	if (this->channel_path != "")
		headers[p].push_back(std::make_pair("Path to sink file"    , this->sink_path                     ));
	if (this->sink_path != "")
		headers[p].push_back(std::make_pair("Path to sink file"    , this->sink_path                     ));
	headers[p].push_back(std::make_pair("Type of source"       , this->src_type                          ));
	if (this->src_path != "")
		headers[p].push_back(std::make_pair("Path to source file"  , this->src_path                      ));
	headers[p].push_back(std::make_pair("Perfect synchronization"         , this->perfect_sync ? "YES" : "NO"         ));
	headers[p].push_back(std::make_pair("Estimator type"       , this->est_type                          ));

	this->p_shp.get_headers(headers);
	this->p_stm.get_headers(headers);
	this->p_sff.get_headers(headers);

	if (full)
		p_rad.get_headers(headers);
}

void DVBS2::
modcod_init(std::string modcod)
{
	if (modcod == "QPSK-S_8/9" || modcod == "")
	{
		mod = "QPSK";
		cod = "8/9";
	}
	else if (modcod == "QPSK-S_3/5"  )
	{
		mod = "QPSK";
		cod = "3/5";
	}
	else if (modcod == "8PSK-S_3/5"  )
	{
		mod = "8PSK";
		cod = "3/5";
		read_order = "TOP_RIGHT";
	}
	else if (modcod == "8PSK-S_8/9"  )
	{
		mod = "8PSK";
		cod = "8/9";
		read_order = "TOP_LEFT";
	}
	else if (modcod == "16APSK-S_8/9")
	{
		mod = "16APSK";
		cod = "8/9";
		read_order = "TOP_LEFT";
	}
	else
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, modcod + " mod-cod scheme not supported.");

	if ( mod == "QPSK"  )
	{
		bps = 2;
		constellation_file = "../conf/mod/4QAM_GRAY.mod";
	}
	else if ( mod == "8PSK"  )
	{
		bps = 3;
		constellation_file = "../conf/mod/8PSK.mod";
	}
	else if ( mod == "16APSK"){
		bps = 4;
		constellation_file = "../conf/mod/16APSK.mod";
	}

	if ( cod == "3/5")
	{
		K_bch             = 9552;
		N_bch             = 9720;
		N_bch_unshortened = 16383;
		src_path          = "../conf/src/K_9552.src";
	}
	else if ( cod == "8/9")
	{
		K_bch             = 14232;
		N_bch             = 14400;
		N_bch_unshortened = 16383;
		src_path          = "../conf/src/K_14232.src";
	}

	itl_n_cols        = bps;
	N_xfec_frame      = N_ldpc / bps;
	N_pilots          = N_xfec_frame / (16 * M);
	S                 = N_xfec_frame / M;
	pl_frame_size     = M * (S + 1) + (N_pilots * P);
}


template <typename B>
module::Source<B>* DVBS2
::build_source(const DVBS2& params, const int seed)
{
	if (params.src_type == "RAND")
		return new module::Source_random_fast<B>(params.K_bch, seed, params.n_frames);
	else if (params.src_type == "USER")
		return new module::Source_user<B>(params.K_bch, params.src_path, params.n_frames);
	else if (params.src_type == "USER_BIN")
		return new module::Source_user_binary<B>(params.K_bch, params.src_path, true, params.src_fifo, params.n_frames);
	else if (params.src_type == "AZCW")
		return new module::Source_AZCW<B>(params.K_bch, params.n_frames);
	else
		throw tools::cannot_allocate(__FILE__, __LINE__, __func__, "Wrong Source type.");

}

template <typename B>
module::Sink<B>* DVBS2
::build_sink(const DVBS2& params)
{
	return new module::Sink_user_binary<B>(params.K_bch, params.sink_path, params.n_frames);
}

template <typename B>
module::Encoder_BCH<B>* DVBS2
::build_bch_encoder(const DVBS2& params, tools::BCH_polynomial_generator<B>& poly_gen)
{
	if (params.n_frames == 1)
		return new module::Encoder_BCH_DVBS2      <B>(params.K_bch, params.N_bch, poly_gen, params.n_frames);
	else
		return new module::Encoder_BCH_inter_DVBS2<B>(params.K_bch, params.N_bch, poly_gen, params.n_frames);
}

template <typename B,typename Q>
module::Decoder_BCH_std<B,Q>* DVBS2
::build_bch_decoder(const DVBS2& params, tools::BCH_polynomial_generator<B>& poly_gen)
{
	return new module::Decoder_BCH_DVBS2<B,Q>(params.K_bch, params.N_bch, poly_gen, params.n_frames);
}

template <typename B,typename Q>
tools::Codec_LDPC<B,Q>* DVBS2
::build_ldpc_cdc(const DVBS2& params)
{
	factory::Codec_LDPC p_cdc;
	auto enc_ldpc = dynamic_cast<factory::Encoder_LDPC*>(p_cdc.enc.get());
	auto dec_ldpc = dynamic_cast<factory::Decoder_LDPC*>(p_cdc.dec.get());

	// store parameters
	enc_ldpc->type          = "LDPC_DVBS2";
	dec_ldpc->type          = "BP_HORIZONTAL_LAYERED";
	p_cdc.enc->n_frames     = params.n_frames;
	dec_ldpc->simd_strategy = params.ldpc_simd;
	dec_ldpc->implem        = params.ldpc_implem;
	enc_ldpc->N_cw          = params.N_ldpc;
	enc_ldpc->K             = params.N_bch;
	dec_ldpc->N_cw          = p_cdc.enc->N_cw;
	dec_ldpc->K             = p_cdc.enc->K;
	dec_ldpc->n_frames      = p_cdc.enc->n_frames;
	enc_ldpc->R             = (float)p_cdc.enc->K / (float)p_cdc.enc->N_cw;
	dec_ldpc->R             = (float)p_cdc.dec->K / (float)p_cdc.dec->N_cw;
	dec_ldpc->n_ite         = params.ldpc_nite;
	p_cdc.K                 = p_cdc.enc->K;
	p_cdc.N_cw              = p_cdc.enc->N_cw;
	p_cdc.N                 = p_cdc.N_cw;
	// build ldpc codec
	return p_cdc.build();
}

template <typename D>
tools::Interleaver_core<D>* DVBS2
::build_itl_core(const DVBS2& params)
{
	if (params.modcod == "QPSK-S_8/9" || params.modcod == "QPSK-S_3/5")
		return new tools::Interleaver_core_NO<D>(params.N_ldpc, params.n_frames);
	else // "8PSK-S_8/9" "8PSK-S_3/5" "16APSK-S_8/9"
		return new tools::Interleaver_core_column_row<D>(params.N_ldpc, params.itl_n_cols, params.read_order, params.n_frames);
}

template <typename D, typename T>
module::Interleaver<D,T>* DVBS2
::build_itl(const DVBS2& params, tools::Interleaver_core<T>& itl_core)
{
	 return new module::Interleaver<D,T>(itl_core);
}

template <typename B, typename R, typename Q, tools::proto_max<Q> MAX, tools::proto_max_i<Q> MAXI>
module::Modem_generic<B,R,Q,MAX>* DVBS2
::build_modem(const DVBS2& params, tools::Constellation<R>* cstl)
{
	// return new module::Modem_generic<B,R,Q,MAX>(params.N_ldpc, *cstl, false, params.n_frames);
	return new module::Modem_generic_fast<B,R,Q,MAX,MAXI>(params.N_ldpc, *cstl, false, params.n_frames);
}

template <typename R>
module::Framer<R>* DVBS2
::build_framer(const DVBS2& params)
{
	return new module::Framer<R>(2 * params.N_ldpc / params.bps, 2 * params.pl_frame_size, params.modcod, params.n_frames);
}

template <typename B>
module::Scrambler_BB<B>* DVBS2
::build_bb_scrambler(const DVBS2& params)
{
	return new module::Scrambler_BB<B>(params.K_bch, params.n_frames);
}

template <typename R>
module::Scrambler_PL<R>* DVBS2
::build_pl_scrambler(const DVBS2& params)
{
	return new module::Scrambler_PL<R>(2*params.pl_frame_size, params.M, params.n_frames);
}

template <typename R>
module::Filter_UPRRC_ccr_naive<R>* DVBS2
::build_uprrc_filter(const DVBS2& params)
{
	return params.p_shp.build_shaping_flt();
}

template <typename R>
module::Filter_Farrow_ccr_naive<R>* DVBS2
::build_channel_frac_delay(const DVBS2& params)
{
	R frac_delay = params.max_delay - std::floor(params.max_delay);
	return new module::Filter_Farrow_ccr_naive <R>(params.pl_frame_size * 2 * params.p_shp.osf,
	                                               frac_delay, params.n_frames);
}

template <typename R>
module::Variable_delay_cc_naive<R>* DVBS2
::build_channel_int_delay(const DVBS2& params)
{
	int N_cplx = params.pl_frame_size * params.p_shp.osf;
	int int_delay = ((int)std::floor(params.max_delay) - 2 + N_cplx) % N_cplx;
	return new module::Variable_delay_cc_naive <R>(N_cplx * 2, int_delay, int_delay, params.n_frames);
}

template <typename R>
module::Filter_buffered_delay<R>* DVBS2
::build_channel_frame_delay(const DVBS2& params)
{
	int int_delay = (int)std::floor(params.max_delay) - 2;
	int N_cplx = params.pl_frame_size * params.p_shp.osf;
	int frame_delay = int_delay / N_cplx;
	return new module::Filter_buffered_delay<R>(N_cplx*2, frame_delay, frame_delay, params.n_frames);
}

template <typename R>
module::Filter_RRC_ccr_naive<R>* DVBS2
::build_matched_filter(const DVBS2& params)
{
	return params.p_shp.build_matched_flt();
}

template <typename R>
module::Estimator<R>* DVBS2
::build_estimator(const DVBS2& params, tools::Noise<R>* noise_ref)
{
	const float code_rate = (float)params.K_bch / (float)params.N_ldpc;
	if (params.est_type == "PERFECT")
		return new module::Estimator_perfect<R>(2 * params.N_xfec_frame, noise_ref, params.n_frames);
	else if (params.est_type == "DVBS2" )
		return new module::Estimator_DVBS2<R>(2 * params.N_xfec_frame, code_rate, params.bps, params.n_frames);

	throw tools::cannot_allocate(__FILE__, __LINE__, __func__, "Wrong Estimator type.");
}

template <typename B>
module::Monitor_BFER<B>* DVBS2
::build_monitor(const DVBS2& params)
{
	// BEGIN DEBUG: ensure that the monitor is making the reduction each time is_done_all() is called
	// Please comment the two following line to increase the simulation throughput
	auto freq = std::chrono::milliseconds(0);

	tools::Monitor_reduction_static::set_reduce_frequency(freq);
	// END DEBUG

	return new module::Monitor_BFER<B>(params.K_bch, params.max_fe, 0, false, params.n_frames);
}

template <typename R>
module::Channel<R>* DVBS2
::build_channel(const DVBS2& params, tools::Gaussian_noise_generator<R>& gen, const bool filtered)
{
	if (params.channel_type == "AWGN")
		if (filtered)
			return new module::Channel_AWGN_LLR<R>(2 * params.pl_frame_size * params.p_shp.osf, gen, false, params.n_frames);
		else
			return new module::Channel_AWGN_LLR<R>(2 * params.pl_frame_size             , gen, false, params.n_frames);
	else if(params.channel_type == "USER_ADD")
		if (filtered)
			return new module::Channel_user_add<R>(2 * params.pl_frame_size * params.p_shp.osf, params.channel_path, true, params.n_frames);
		else
			return new module::Channel_user_add<R>(2 * params.pl_frame_size             , params.channel_path, true, params.n_frames);
	else

	throw tools::cannot_allocate(__FILE__, __LINE__, __func__, "Wrong Channel type.");
}

template <typename R>
module::Multiplier_fading_DVBS2<R>* DVBS2
::build_fading_mult(const DVBS2& params)
{
	return new module::Multiplier_fading_DVBS2<R>(2*params.pl_frame_size * params.p_shp.osf, params.esn0_seq_path, params.esn0_ref, params.n_frames);
}

template <typename R>
module::Multiplier_sine_ccc_naive<R>* DVBS2
::build_freq_shift(const DVBS2& params)
{
	return new module::Multiplier_sine_ccc_naive<R>(2*params.pl_frame_size * params.p_shp.osf, params.max_freq_shift, 1.0,
	                                                params.n_frames);
}

template <typename R>
module::Synchronizer_freq_fine<R>* DVBS2
::build_synchronizer_lr(const DVBS2& params)
{
	return params.p_sff.build_lr();

}

template <typename R>
module::Synchronizer_freq_fine<R>* DVBS2
::build_synchronizer_freq_phase(const DVBS2& params)
{
	return params.p_sff.build_freq_phase();
}

template <typename B, typename R>
module::Synchronizer_timing<B,R>* DVBS2
::build_synchronizer_timing(const DVBS2& params)
{
	return params.p_stm.build();
}


template <typename R>
module::Multiplier_AGC_cc_naive<R>* DVBS2
::build_agc_shift(const DVBS2& params)
{
	return new module::Multiplier_AGC_cc_naive<R>(2 * params.pl_frame_size, (R)1, params.n_frames);
}

template <typename R>
module::Multiplier_AGC_cc_naive<R>* DVBS2
::build_channel_agc(const DVBS2& params)
{
	return new module::Multiplier_AGC_cc_naive<R>(2 * params.pl_frame_size * params.p_shp.osf, 1.0/(R)params.p_shp.osf, params.n_frames);
}

template <typename R>
module::Synchronizer_frame<R>* DVBS2
::build_synchronizer_frame(const DVBS2& params)
{
	if (params.sfm_type == "PERFECT")
	{
		int delay = (R)2 * (R)params.p_shp.grp_delay + ((int)std::floor(params.max_delay) + 1)/params.p_shp.osf;
		return dynamic_cast<module::Synchronizer_frame<R>*>(new module::Synchronizer_frame_perfect<R>(2 * params.pl_frame_size, delay, params.n_frames));
	}
	else if (params.sfm_type == "FAST")
	{
		return dynamic_cast<module::Synchronizer_frame<R>*>(new module::Synchronizer_frame_DVBS2_fast<R>(2 * params.pl_frame_size, params.sfm_alpha, params.sfm_trigger, params.n_frames));
	}
	else if (params.sfm_type == "NORMAL")
	{
		return dynamic_cast<module::Synchronizer_frame<R>*>(new module::Synchronizer_frame_DVBS2_aib <R>(2 * params.pl_frame_size, params.sfm_alpha, params.sfm_trigger, params.n_frames));

	}
	else
	{
		throw tools::cannot_allocate(__FILE__, __LINE__, __func__, "Wrong Synchronizer_frame type.");
	}

}

template <typename R>
module::Synchronizer_freq_coarse<R>* DVBS2
::build_synchronizer_freq_coarse(const DVBS2& params)
{
	module::Synchronizer_freq_coarse<R> * sync_freq_coarse;
	if (params.perfect_coarse_freq_sync)
		sync_freq_coarse =  dynamic_cast<module::Synchronizer_freq_coarse<R>*>(new module::Synchronizer_freq_coarse_perfect<R>(2 * params.pl_frame_size * params.p_shp.osf, params.max_freq_shift, params.n_frames));
	else
		sync_freq_coarse =  dynamic_cast<module::Synchronizer_freq_coarse<R>*>(new module::Synchronizer_freq_coarse_DVBS2_aib<R>(2 * params.pl_frame_size * params.p_shp.osf, params.p_shp.osf, 0.707, 1e-4, params.n_frames));

	return sync_freq_coarse;
}

template <typename B>
module::Filter_buffered_delay<B>* DVBS2
::build_txrx_delay(const DVBS2& params)
{
	return new module::Filter_buffered_delay<B>(params.K_bch, params.overall_delay + 100, params.overall_delay, params.n_frames);
}

template <typename B, typename R>
module::Synchronizer_step_mf_cc<B, R>* DVBS2
::build_synchronizer_step_mf_cc(const DVBS2& params,
	                            aff3ct::module::Synchronizer_freq_coarse<R> *sync_coarse_f,
	                            aff3ct::module::Filter_RRC_ccr_naive<R>     *matched_filter,
	                            aff3ct::module::Synchronizer_timing<B,R>    *sync_timing)
{
	return new module::Synchronizer_step_mf_cc<B,R>(sync_coarse_f, matched_filter, sync_timing, params.n_frames);
}

template <typename R>
module::Radio<R>* DVBS2
::build_radio (const DVBS2& params)
{
	return params.p_rad.build<R>();
}

template aff3ct::module::Source<B>*                    DVBS2::build_source<B>                  (const DVBS2& params, const int seed);
template aff3ct::module::Sink<B>*                      DVBS2::build_sink<B>                    (const DVBS2& params);
template aff3ct::module::Encoder_BCH<B>*               DVBS2::build_bch_encoder<B>             (const DVBS2& params, tools::BCH_polynomial_generator<B>& poly_gen);
template aff3ct::module::Decoder_BCH_std<B>*           DVBS2::build_bch_decoder<B>             (const DVBS2& params, tools::BCH_polynomial_generator<B>& poly_gen);
template aff3ct::tools ::Codec_LDPC<B,Q>*              DVBS2::build_ldpc_cdc<B,Q>              (const DVBS2& params);
template aff3ct::module::Interleaver<int32_t,uint32_t>*DVBS2::build_itl<int32_t,uint32_t>      (const DVBS2& params, tools::Interleaver_core<uint32_t>& itl_core);
template aff3ct::module::Interleaver<float,uint32_t>*  DVBS2::build_itl<float,uint32_t>        (const DVBS2& params, tools::Interleaver_core<uint32_t>& itl_core);
template aff3ct::module::Modem_generic<B,R,Q,tools::max_star>* DVBS2::build_modem              (const DVBS2& params, tools::Constellation<R>* cstl);
template aff3ct::module::Modem_generic<B,R,Q,tools::max     >* DVBS2::build_modem              (const DVBS2& params, tools::Constellation<R>* cstl);
template aff3ct::module::Framer<R>*                    DVBS2::build_framer                     (const DVBS2& params);
template aff3ct::module::Scrambler_BB<B>*              DVBS2::build_bb_scrambler<B>            (const DVBS2& params);
template aff3ct::module::Scrambler_PL<R>*              DVBS2::build_pl_scrambler<R>            (const DVBS2& params);
template aff3ct::module::Filter_UPRRC_ccr_naive<R>*    DVBS2::build_uprrc_filter<R>            (const DVBS2& params);
template aff3ct::module::Filter_Farrow_ccr_naive<R>*   DVBS2::build_channel_frac_delay<R>      (const DVBS2& params);
template aff3ct::module::Variable_delay_cc_naive<R>*   DVBS2::build_channel_int_delay<R>       (const DVBS2& params);
template aff3ct::module::Filter_buffered_delay<R>*     DVBS2::build_channel_frame_delay<R>     (const DVBS2& params);
template aff3ct::module::Filter_RRC_ccr_naive<R>*      DVBS2::build_matched_filter<R>          (const DVBS2& params);
template aff3ct::module::Estimator<R>*                 DVBS2::build_estimator<R>               (const DVBS2& params, tools::Noise<R>* noise_ref);
template aff3ct::module::Monitor_BFER<B>*              DVBS2::build_monitor<B>                 (const DVBS2& params);
template aff3ct::module::Channel<R>*                   DVBS2::build_channel<R>                 (const DVBS2& params, tools::Gaussian_noise_generator<R>& gen, const bool filtered);
template aff3ct::module::Multiplier_sine_ccc_naive<R>* DVBS2::build_freq_shift<R>              (const DVBS2& params);
template aff3ct::module::Multiplier_fading_DVBS2<R>*   DVBS2::build_fading_mult<R>             (const DVBS2& params);
template aff3ct::module::Synchronizer_freq_fine<R>*    DVBS2::build_synchronizer_lr<R>         (const DVBS2& params);
template aff3ct::module::Synchronizer_freq_fine<R>*    DVBS2::build_synchronizer_freq_phase<R> (const DVBS2& params);
template aff3ct::module::Synchronizer_timing<B,R>*     DVBS2::build_synchronizer_timing<B,R>   (const DVBS2& params);
template aff3ct::module::Multiplier_AGC_cc_naive<R>*   DVBS2::build_agc_shift<R>               (const DVBS2& params);
template aff3ct::module::Multiplier_AGC_cc_naive<R>*   DVBS2::build_channel_agc<R>             (const DVBS2& params);
template aff3ct::module::Synchronizer_frame<R>*        DVBS2::build_synchronizer_frame<R>      (const DVBS2& params);
template aff3ct::module::Synchronizer_freq_coarse<R>*  DVBS2::build_synchronizer_freq_coarse<R>(const DVBS2& params);
template aff3ct::module::Filter_buffered_delay<B>*     DVBS2::build_txrx_delay<B>              (const DVBS2& params);
template aff3ct::tools ::Interleaver_core<uint32_t>*   DVBS2::build_itl_core<uint32_t>         (const DVBS2& params);
template aff3ct::module::Synchronizer_step_mf_cc<B,R>* DVBS2::build_synchronizer_step_mf_cc<B,R>(const DVBS2& params,
                                                                                                 aff3ct::module::Synchronizer_freq_coarse<R> *sync_coarse_f,
                                                                                                 aff3ct::module::Filter_RRC_ccr_naive<R>     *matched_filter,
                                                                                                 aff3ct::module::Synchronizer_timing<B,R>    *sync_timing   );
template aff3ct::module::Radio<R>*                             DVBS2::build_radio<R>           (const DVBS2& params);

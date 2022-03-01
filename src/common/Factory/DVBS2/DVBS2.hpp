#ifndef FACTORY_DVBS2_HPP
#define FACTORY_DVBS2_HPP

#include <aff3ct.hpp>

#include "Factory/DVBS2/DVBS2.hpp"

#include "Module/Multiplier/Sequence/Multiplier_fading_DVBS2.hpp"
#include "Tools/Display/rang_format/rang_format.h"

#include "Factory/Module/Radio/Radio.hpp"
#include "Factory/Module/Synchronizer_freq_fine/Synchronizer_freq_fine.hpp"
#include "Factory/Module/Synchronizer_freq_coarse/Synchronizer_freq_coarse.hpp"
#include "Factory/Module/Synchronizer_timing/Synchronizer_timing.hpp"
#include "Factory/Module/Shaping_filter/Shaping_filter.hpp"
#include "Factory/Module/Synchronizer_frame/Synchronizer_frame.hpp"

#include "Module/Framer/Framer.hpp"
#include "Module/Scrambler/Scrambler_BB/Scrambler_BB.hpp"
#include "Module/Scrambler/Scrambler_PL/Scrambler_PL.hpp"
#include "Module/Filter/Filter_UPFIR/Filter_UPRRC/Filter_UPRRC_ccr_naive.hpp"
#include "Module/Filter/Filter_unit_delay/Filter_buffered_delay.hpp"
#include "Module/Filter/Filter_FIR/Filter_RRC/Filter_RRC_ccr_naive.hpp"
#include "Module/Filter/Filter_FIR/Farrow/Filter_Farrow_ccr_naive.hpp"
#include "Module/Filter/Variable_delay/Variable_delay_cc_naive.hpp"
#include "Module/Multiplier/Sine/Multiplier_sine_ccc_naive.hpp"
#include "Module/Multiplier/Sequence/Multiplier_AGC_cc_naive.hpp"
#include "Module/Synchronizer/Synchronizer_freq/Synchronizer_freq_coarse/Synchronizer_freq_coarse.hpp"
#include "Module/Synchronizer/Synchronizer_freq/Synchronizer_freq_fine/Synchronizer_freq_fine.hpp"
#include "Module/Synchronizer/Synchronizer_frame/Synchronizer_frame.hpp"
#include "Module/Synchronizer/Synchronizer_step_mf_cc.hpp"
#include "Module/Estimator/Estimator.hpp"
#include "Module/Radio/Radio.hpp"
#include "Module/Sink/Sink.hpp"
#include "Module/Feedbacker/Feedbacker.hpp"

namespace aff3ct
{
namespace factory
{

extern const std::string DVBS2_name;
extern const std::string DVBS2_prefix;

struct DVBS2 : Factory
{
public:
	// const
	const int N_ldpc = 16200;
	const int M      = 90;    // number of symbols per slot
	const int P      = 36;    // number of symbols per pilot

	const std::vector<float> pilot_values = std::vector<float  > (P*2, std::sqrt(2.0f)/2.0f);
	const std::vector<int  > pilot_start  = {1530, 3006, 4482, 5958, 7434};
	const std::vector<int  > bch_prim_poly = {1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1};

	const std::string mat2aff_file_name = "../build/matlab_to_aff3ct.txt";
	const std::string aff2mat_file_name = "../build/aff3ct_to_matlab.txt";

	// no const
	float ebn0_min;
	float ebn0_max;
	float ebn0_step;
	float esn0_ref;

	float max_freq_shift;
	float max_delay;
	int   overall_delay;
	bool  debug;
	bool  stats;
	bool  src_fifo;
	bool  display_help;
	bool  perfect_sync;

	int   max_fe;       // max number of frame errors per SNR point
	int   max_n_frames; // max number of simulated frames per SNR point
	int   K_bch;
	int   N_bch;
	int   N_bch_unshortened;
	int   ldpc_nite;
	int   bps;
	int   N_xfec_frame; // number of complex symbols
	int   N_pilots;
	int   S;            // number of slots
	int   pl_frame_size;
	int   itl_n_cols;
	int   n_frames;
	int   debug_limit;

	std::chrono::milliseconds ter_freq;

	std::string ldpc_implem;
	std::string ldpc_simd;
	std::string read_order;
	std::string modcod;
	std::string mod;
	std::string cod;
	std::string constellation_file;
	std::string section;
	std::string src_type;
	std::string src_path;
	std::string sink_path;
	std::string channel_path;
	std::string esn0_seq_path;
	std::string est_type;
	std::string channel_type;
	std::string dump_filename;

	factory::Radio                    p_rad;
	factory::Synchronizer_freq_fine   p_sff;
	factory::Synchronizer_freq_coarse p_sfc;
	factory::Synchronizer_timing      p_stm;
	factory::Synchronizer_frame       p_sfm;
	factory::Shaping_filter           p_shp;

private:
	void modcod_init(std::string modcod);

public:
	DVBS2(int argc, char** argv, const std::string &p = DVBS2_prefix);
	DVBS2* clone() const;

	virtual std::vector<std::string> get_names      () const;
	virtual std::vector<std::string> get_short_names() const;
	virtual std::vector<std::string> get_prefixes   () const;

	// parameters construction
	void get_description(cli::Argument_map_info &args) const;
	void store          (const cli::Argument_map_value &vals);
	void get_headers    (std::map<std::string,tools::header_list>& headers, const bool full = true) const;

public:
	template <typename B = int>
	static module::Source<B>*
	build_source(const DVBS2& params, const int seed = 0);

	template <typename B = int>
	static module::Sink<B>*
	build_sink(const DVBS2& params);

	template <typename B = int>
	static module::Encoder_BCH<B>*
	build_bch_encoder(const DVBS2& params,
	                  tools::BCH_polynomial_generator<B>& poly_gen);

	template <typename B = int, typename Q = float>
	static module::Decoder_BCH_std<B,Q>*
	build_bch_decoder(const DVBS2& params,tools::BCH_polynomial_generator<B>& poly_gen);

	template <typename B = int, typename Q = float>
	static tools::Codec_LDPC<B,Q>*
	build_ldpc_cdc(const DVBS2& params);

	template <typename D = uint32_t>
	static tools::Interleaver_core<D>*
	build_itl_core(const DVBS2& params);

	template <typename D = int32_t, typename T = uint32_t>
	static module::Interleaver<D,T>*
	build_itl(const DVBS2& params, tools::Interleaver_core<T>& itl_core);

	template <typename B = int, typename R = float, typename Q = R, tools::proto_max  <Q> MAX  = tools::max_star,
	                                                                tools::proto_max_i<Q> MAXI = tools::max_star_i>
	static module::Modem_generic<B,R,Q,MAX>*
	build_modem(const DVBS2& params, tools::Constellation<R>* cstl);

	template <typename R = float>
	static module::Multiplier_sine_ccc_naive<R>*
	build_freq_shift(const DVBS2& params);

	template <typename R = float>
	static module::Multiplier_fading_DVBS2<R>*
	build_fading_mult(const DVBS2& params);

	template <typename R = float>
	static module::Channel<R>*
	build_channel(const DVBS2& params, tools::Gaussian_noise_generator<R>& gen, const bool filtered = true);

	template <typename R = float>
	static module::Framer<R>*
	build_framer(const DVBS2& params);

	template <typename B = int>
	static module::Scrambler_BB<B>*
	build_bb_scrambler(const DVBS2& params);

	template <typename R = float>
	static module::Scrambler_PL<R>*
	build_pl_scrambler(const DVBS2& params);

	template <typename R = float>
	static module::Filter_UPRRC_ccr_naive<R>*
	build_uprrc_filter(const DVBS2& params);

	template <typename R = float>
	static module::Filter_Farrow_ccr_naive<R>*
	build_channel_frac_delay(const DVBS2& params);

	template <typename R = float>
	static module::Variable_delay_cc_naive<R>*
	build_channel_int_delay(const DVBS2& params);

	template <typename R = float>
	static module::Filter_buffered_delay<R>*
	build_channel_frame_delay(const DVBS2& params);

	template <typename R = float>
	static module::Filter_RRC_ccr_naive<R>*
	build_matched_filter(const DVBS2& params);

	template <typename R = float>
	static module::Estimator<R>*
	build_estimator(const DVBS2& params, tools::Noise<R>* noise_ref = nullptr);

	template <typename R = float>
	static module::Synchronizer_freq_fine<R>*
	build_synchronizer_lr(const DVBS2& params);

	template <typename R = float>
	static module::Synchronizer_freq_fine<R>*
	build_synchronizer_freq_phase(const DVBS2& params);

	template <typename B = int, typename R = float>
	static module::Synchronizer_timing<B, R>*
	build_synchronizer_timing (const DVBS2& params);

	template <typename R = float>
	static module::Multiplier_AGC_cc_naive<R>*
	build_agc_shift(const DVBS2& params);

	template <typename R = float>
	static module::Multiplier_AGC_cc_naive<R>*
	build_channel_agc(const DVBS2& params);

	template <typename R = float>
	static module::Synchronizer_frame<R>*
	build_synchronizer_frame (const DVBS2& params);

	template <typename R = float>
	static module::Synchronizer_freq_coarse<R>*
	build_synchronizer_freq_coarse (const DVBS2& params);

	template <typename B = int, typename R = float>
	static module::Synchronizer_step_mf_cc<B,R>*
	build_synchronizer_step_mf_cc(const DVBS2& params,
	                              aff3ct::module::Synchronizer_freq_coarse<R> *sync_coarse_f,
	                              aff3ct::module::Filter_RRC_ccr_naive<R>     *matched_filter,
	                              aff3ct::module::Synchronizer_timing<B,R>    *sync_timing  );


	template <typename B = int>
	static module::Filter_buffered_delay<B>*
	build_txrx_delay(const DVBS2& params);

	template <typename B = int>
	static module::Monitor_BFER<B>*
	build_monitor(const DVBS2& params);

	template <typename R = float>
	static module::Radio<R>*
	build_radio(const DVBS2& params);

	template <typename D = int>
	static module::Feedbacker<D>*
	build_feedbacker(const DVBS2& params);
};
}
}
#endif /* FACTORY_DVBS2_HPP */

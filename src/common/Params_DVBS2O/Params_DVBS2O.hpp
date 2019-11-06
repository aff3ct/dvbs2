#ifndef PARAMS_DVBS2O_HPP
#define PARAMS_DVBS2O_HPP

#include <aff3ct.hpp>
#include "Factory/Module/Radio/Radio.hpp"
using namespace aff3ct;
using namespace factory;

struct Params_DVBS2O
{
public:
	factory::Radio p_rad;

	float ebn0_min;
	float ebn0_max;
	float ebn0_step;
	bool debug;
	bool stats;
	bool filtered;    // filtering disabled in bb simu
	bool no_pll;
	int max_fe;       // max number of frame errors per SNR point
	int max_n_frames; // max number of simulated frames per SNR point
	int K_bch;
	int N_bch;
	int N_bch_unshortened;
	int ldpc_nite;
	int bps;
	int N_xfec_frame;              // number of complex symbols
	int N_pilots;
	int S;                         // number of slots
	int pl_frame_size;
	int itl_n_cols;
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

	const int   N_ldpc    = 16200;
	const int   M         = 90;    // number of symbols per slot
	const int   P         = 36;    // number of symbols per pilot
	const float rolloff   = 0.2;//DVBS2 0.05; // DVBS2-X
	const int   osf       = 4;
	const int   grp_delay = 50;

	float max_freq_shift;
	float max_delay;

	const std::vector<float  > pilot_values = std::vector<float  > (P*2, std::sqrt(2.0f)/2.0f);
	const std::vector<int    > pilot_start  = {1530, 3006, 4482, 5958, 7434};

	const std::string mat2aff_file_name = "../build/matlab_to_aff3ct.txt";
	const std::string aff2mat_file_name = "../build/aff3ct_to_matlab.txt";

	const std::vector<int> bch_prim_poly = {1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1};

	Params_DVBS2O(int argc, char** argv);
	void get_arguments(int argc, char** argv, cli::Argument_map_value& arg_vals);
};

#endif // PARAMS_DVBS2O_HPP

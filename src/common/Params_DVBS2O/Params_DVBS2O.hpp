#ifndef PARAMS_DVBS2O_HPP
#define PARAMS_DVBS2O_HPP

#include <aff3ct.hpp>

using namespace aff3ct;

class Params_DVBS2O
{
public:
	float ebn0_min;
	float ebn0_max;
	float ebn0_step;
	bool stats;
	int MAX_FE;                    // max number of frame errors per SNR point
	int MAX_N_FRAMES;              // max number of simulated frames per SNR point
	int K_BCH;
	int N_BCH;
	int N_BCH_unshortened;
	int K_LDPC;
	int LDPC_NITE;
	int BPS;
	int N_XFEC_FRAME;              // number of complex symbols
	int N_PILOTS;
	int S;                         // number of slots
	int PL_FRAME_SIZE;
	int ITL_N_COLS;
	std::string LDPC_IMPLEM;
	std::string LDPC_SIMD;
	std::string READ_ORDER;
	std::string MODCOD;
	std::string MOD;
	std::string COD;
	std::string constellation_file;
	std::string section;

	const int   N_LDPC    = 16200;
	const int   M         = 90;    // number of symbols per slot
	const int   P         = 36;    // number of symbols per pilot
	const float ROLLOFF   = 0.05;
	const int   OSF       = 4;
	const int   GRP_DELAY = 50;

	float MAX_FREQ_SHIFT;
	float MAX_DELAY;

	const std::vector<float  > pilot_values = std::vector<float  > (P*2, std::sqrt(2.0f)/2.0f);
	const std::vector<int    > pilot_start  = {1530, 3006, 4482, 5958, 7434};

	const std::string mat2aff_file_name = "../build/matlab_to_aff3ct.txt";
	const std::string aff2mat_file_name = "../build/aff3ct_to_matlab.txt";

	const std::vector<int> bch_prim_poly = {1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1};

	Params_DVBS2O(int argc, char** argv);
	void get_arguments(int argc, char** argv, tools::Argument_map_value& arg_vals);


};

#endif // PARAMS_DVBS2O_HPP

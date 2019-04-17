#ifndef DVBS2_PARAMS_HPP
#define DVBS2_PARAMS_HPP

#include <aff3ct.hpp>

using namespace aff3ct;

class DVBS2_params
{
public:
	int K_BCH;
	int N_BCH;
	int N_BCH_unshortened;
	int K_LDPC;
	int BPS;
	int N_XFEC_FRAME;              // number of complex symbols
	int N_PILOTS;
	int S;                         // number of slots
	int PL_FRAME_SIZE;

	const int   N_LDPC    = 16200;
	const int   M         = 90;    // number of symbols per slot
	const int   P         = 36;    // number of symbols per pilot
	const float ROLLOFF   = 0.05;
	const int   OSF       = 4;
	const int   GRP_DELAY = 50;
	const std::vector<int > BCH_gen_poly {1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0,
	                                      1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1,
	                                      1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1,
	                                      1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0,
	                                      1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1,
	                                      1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0,
	                                      1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0,
	                                      0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	                                      0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1,
	                                      1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0,
	                                      0, 0, 0, 0, 0, 0, 1, 0, 1};

	const std::string mat2aff_file_name = "../build/matlab_to_aff3ct.txt";
	const std::string aff2mat_file_name = "../build/aff3ct_to_matlab.txt";

	DVBS2_params(int argc, char** argv);
	void get_arguments(int argc, char** argv, tools::Argument_map_value& arg_vals);


};

#endif // DVBS2_PARAMS_HPP

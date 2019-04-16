#ifndef DVBS2_PARAMS_HPP
#define DVBS2_PARAMS_HPP

using namespace aff3ct;

class DVBS2_params
{
public:
	int K_BCH;
	int N_BCH;
	int N_LDPC;
	int K_LDPC;
	int BPS;
	int M;             // number of symbols per slot
	int P;             // number of symbols per pilot

	int N_XFEC_FRAME;  // number of complex symbols
	int N_PILOTS;
	int S;             // number of slots
	int PL_FRAME_SIZE;
	
	float ROLLOFF;
	int   N_SYMBOLS;
	int   OSF;
	int   GRP_DELAY;
	const std::vector<int > BCH_gen_poly{1, 0, 1, 0, 0, 1, 0, 1,
	                                     1, 0, 1, 0, 0, 0, 0, 0,
	                                     1, 0, 0, 1, 1, 0, 0, 0,
	                                     1, 0, 0, 0, 1, 0, 1, 1,
	                                     1, 1, 1, 0, 1, 0, 1, 1,
	                                     1, 1, 1, 0, 0, 1, 1, 1,
	                                     1, 1, 1, 1, 0, 0, 0, 1,
	                                     0, 1, 0, 0, 1, 0, 1, 0,
	                                     1, 0, 0, 1, 0, 1, 1, 0,
	                                     0, 0, 0, 0, 1, 0, 0, 1,
	                                     1, 1, 0, 0, 0, 1, 0, 1,
	                                     1, 1, 0, 0, 0, 1, 0, 0,
	                                     1, 0, 1, 1, 0, 0, 1, 1,
	                                     0, 1, 0, 0, 0, 1, 1, 0,
	                                     0, 1, 0, 0, 1, 1, 0, 1,
	                                     1, 0, 0, 1, 0, 1, 1, 0,
	                                     0, 0, 0, 1, 1, 0, 0, 1,
	                                     0, 1, 0, 1, 0, 1, 1, 1,
	                                     1, 1, 0, 1, 1, 0, 1, 1,
	                                     0, 1, 0, 0, 0, 1, 1, 0,
	                                     0, 0, 0, 0, 0, 0, 1, 0,
	                                     1};
	DVBS2_params() {}

	void init(std::string modcod = "QPSK-S_8/9")
	{
		if (modcod == "QPSK-S_8/9" || modcod == "")
		{
			K_BCH  = 14232;
			N_BCH  = 14400;
			N_LDPC = 16200;
			K_LDPC = N_BCH;
			BPS    = 2;
			M      = 90;
			P      = 36;

			N_XFEC_FRAME  = N_LDPC / BPS;
			N_PILOTS      = N_XFEC_FRAME / (16 * M);
			S             = N_XFEC_FRAME / 90;
			PL_FRAME_SIZE = M * (S + 1) + (N_PILOTS * P);

			ROLLOFF   = 0.05;
			N_SYMBOLS = 8370;
			OSF       = 4;
			GRP_DELAY = 50;
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

};

#endif // DVBS2_PARAMS_HPP

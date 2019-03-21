#include <vector>
#include <iostream>

#include <aff3ct.hpp>
using namespace aff3ct::module;

#include "Sink.hpp"
#include "BB_scrambler.hpp"
#include "../common/PL_scrambler/PL_scrambler.hpp"


#include "Filter/Filter_UPFIR/Filter_UPRRC/Filter_UPRRC_ccr_naive.hpp"

int main(int argc, char** argv)
{
	using namespace aff3ct;

	const std::string mat2aff_file_name = "../build/matlab_to_aff3ct.txt";
	const std::string aff2mat_file_name = "../build/aff3ct_to_matlab.txt";

	Sink sink_to_matlab    (mat2aff_file_name, aff2mat_file_name);

	const int K_BCH = 14232;
	const int N_BCH = 14400;
	const int N_LDPC = 16200;
	const int K_LDPC = N_BCH;
	const int BPS = 2; // QPSK
	const int M = 90; // number of symbols per slot
	const int P = 36; // number of symbols per pilot

	const int N_XFEC_FRAME = N_LDPC / BPS; // number of complex symbols
	const int N_PILOTS = N_XFEC_FRAME / (16*M);
	const int S = N_XFEC_FRAME / 90; // number of slots	
	const int PL_FRAME_SIZE = M*(S+1) + (N_PILOTS*P);

	// Tracer
	tools::Frame_trace<>     tracer            (20, 5, std::cout);

	if (sink_to_matlab.destination_chain_name == "descramble")
	{

		PL_scrambler<float> complex_scrambler(2*PL_FRAME_SIZE, M, false);
		std::vector<float  > SCRAMBLED_PL_FRAME(2*PL_FRAME_SIZE);
		//std::vector<float  > PL_FRAME(2*PL_FRAME_SIZE);
		std::vector<float  > PL_FRAME_OUTPUT(2*PL_FRAME_SIZE);


		std::vector<float> PL_FRAME(2*PL_FRAME_SIZE-2*M);

		

		sink_to_matlab.pull_vector( SCRAMBLED_PL_FRAME );

		PL_FRAME.insert(PL_FRAME.begin(), SCRAMBLED_PL_FRAME.begin(), SCRAMBLED_PL_FRAME.begin()+2*M);

		complex_scrambler.scramble(SCRAMBLED_PL_FRAME, PL_FRAME);
		//tracer.display_real_vector(SCRAMBLED_PL_FRAME);
		//std::copy(PL_FRAME.begin(), PL_FRAME.end(), PL_FRAME_OUTPUT.begin());
		sink_to_matlab.push_vector( PL_FRAME , true);
	}
	else if (sink_to_matlab.destination_chain_name == "deframe")
	{
		std::vector<float  > mod_samples(2*PL_FRAME_SIZE);
		std::vector<float  > mod_samples_out(2*N_XFEC_FRAME);
		
		sink_to_matlab.pull_vector( mod_samples );

		mod_samples.erase(mod_samples.begin(), mod_samples.begin() + 2*M); // erase the PLHEADER

		for( int i = 1; i < N_PILOTS+1; i++)
		{
			mod_samples.erase(mod_samples.begin()+(i*90*16*2), mod_samples.begin()+(i*90*16*2)+(36*2) );
		}
		mod_samples_out = mod_samples;

		sink_to_matlab.push_vector( mod_samples_out , true);


	}
	else if (sink_to_matlab.destination_chain_name == "decoding")
	{

		const std::vector<int > BCH_gen_poly{1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1};

		// buffers to store the data
		std::vector<int  > scrambler_in(K_BCH);	
		std::vector<int  > BCH_encoded(N_BCH);
		std::vector<float  > LDPC_encoded(N_LDPC);
		std::vector<int  > parity(N_BCH-K_BCH);
		std::vector<int  > msg(K_BCH);

		// Base Band scrambler
		BB_scrambler             my_scrambler;
		////////////////////////////////////////////////////
		// retrieve data from Matlab
		////////////////////////////////////////////////////
		//sink_to_matlab.pull_vector( LDPC_encoded );
		sink_to_matlab.pull_vector( BCH_encoded );

		//auto dvbs2 = tools::build_dvbs2(K_LDPC, N_LDPC);

		//tools::Sparse_matrix H_dvbs2;
		//H_dvbs2 = build_H(*dvbs2);

		//std::vector<uint32_t> info_bits_pos(K_LDPC);

		//for(int i = 0; i< K_LDPC; i++)
		//	info_bits_pos[i] = i+N_LDPC-K_LDPC;

		//Decoder_LDPC_BP_flooding_SPA<int, float> LDPC_decoder(K_LDPC, N_LDPC, 20, H_dvbs2, info_bits_pos, false, 1);

		//LDPC_decoder.decode_siho(LDPC_encoded, BCH_encoded);
		
		// BCH decoding

		tools::BCH_polynomial_generator<int  > poly_gen(16383, 12);
		poly_gen.set_g(BCH_gen_poly);
		Decoder_BCH_std<int> BCH_decoder(K_BCH, N_BCH, poly_gen);

		parity.assign(BCH_encoded.begin()+K_BCH, BCH_encoded.begin()+N_BCH); // retrieve parity
		std::reverse(parity.begin(), parity.end()); // revert parity bits

		msg.assign(BCH_encoded.begin(), BCH_encoded.begin()+K_BCH); // retrieve message
		std::reverse(msg.begin(), msg.end()); // revert msg bits

		BCH_encoded.insert(BCH_encoded.begin(), parity.begin(), parity.end());
		BCH_encoded.insert(BCH_encoded.begin()+(N_BCH-K_BCH), msg.begin(), msg.end());

		BCH_encoded.erase(BCH_encoded.begin()+N_BCH, BCH_encoded.end());

		BCH_decoder.decode_hiho(BCH_encoded, scrambler_in);

		std::reverse(scrambler_in.begin(), scrambler_in.end());

		// BB descrambling
		my_scrambler.scramble(scrambler_in);

		sink_to_matlab.push_vector( scrambler_in , false);
		//sink_to_matlab.push_vector( BCH_encoded , false);

	}

	
	else
	{
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, "Invalid destination name.");
	}

	return 0;
}
#include <vector>
#include <iostream>

#include <aff3ct.hpp>
using namespace aff3ct::module;

#include "Sink.hpp"
#include "BB_scrambler.hpp"

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

	if (sink_to_matlab.destination_chain_name == "coding")
	{

		const std::vector<int > BCH_gen_poly{1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1};

		// buffers to store the data
		std::vector<int  > scrambler_in(K_BCH);	
		std::vector<int  > BCH_encoded(N_BCH);
		std::vector<float  > LDPC_encoded(N_LDPC);
		std::vector<int  > parity(N_BCH-K_BCH);
		std::vector<int  > msg(K_BCH);

		// Tracer
		tools::Frame_trace<>     tracer            (20, 5, std::cout);

		// Base Band scrambler
		BB_scrambler             my_scrambler;
		////////////////////////////////////////////////////
		// retrieve data from Matlab
		////////////////////////////////////////////////////
		//sink_to_matlab.pull_vector( LDPC_encoded );
		sink_to_matlab.pull_vector( BCH_encoded );

		//auto dvbs2 = tools::build_dvbs2(N_BCH, N_LDPC);

		//tools::Sparse_matrix H_dvbs2;
		//H_dvbs2 = build_H(*dvbs2);

		//std::vector<uint32_t> info_bits_pos(N_BCH);

		//for(int i = 0; i< N_BCH; i++)
		//	info_bits_pos[i] = i;

		//for(int i = 0; i< N_LDPC; i++)
		//	LDPC_encoded[i] = -1*LDPC_encoded[i];

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
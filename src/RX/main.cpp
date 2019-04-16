#include <vector>
#include <iostream>

#include <aff3ct.hpp>

#include "Sink.hpp"
#include "BB_scrambler.hpp"
#include "../common/PL_scrambler/PL_scrambler.hpp"
#include "Filter/Filter_UPFIR/Filter_UPRRC/Filter_UPRRC_ccr_naive.hpp"
#include "DVBS2_params.hpp"

using namespace aff3ct;

int main(int argc, char** argv)
{
	auto params = DVBS2_params(argc, argv);

	Sink sink_to_matlab    (params.mat2aff_file_name, params.aff2mat_file_name);

	tools::Frame_trace<>     tracer            (20, 5, std::cout);

	if (sink_to_matlab.destination_chain_name == "descramble")
	{
		module::PL_scrambler<float> complex_scrambler(2*params.PL_FRAME_SIZE, params.M, false);
		std::vector<float  > SCRAMBLED_PL_FRAME(2*params.PL_FRAME_SIZE);
		//std::vector<float  > PL_FRAME(2*params.PL_FRAME_SIZE);
		std::vector<float  > PL_FRAME_OUTPUT(2*params.PL_FRAME_SIZE);

		std::vector<float> PL_FRAME(2*params.PL_FRAME_SIZE-2*params.M);

		sink_to_matlab.pull_vector( SCRAMBLED_PL_FRAME );

		PL_FRAME.insert(PL_FRAME.begin(), SCRAMBLED_PL_FRAME.begin(), SCRAMBLED_PL_FRAME.begin()+2*params.M);

		complex_scrambler.scramble(SCRAMBLED_PL_FRAME, PL_FRAME);
		//tracer.display_real_vector(SCRAMBLED_PL_FRAME);
		//std::copy(PL_FRAME.begin(), PL_FRAME.end(), PL_FRAME_OUTPUT.begin());
		sink_to_matlab.push_vector( PL_FRAME , true);
	}
	else if (sink_to_matlab.destination_chain_name == "deframe")
	{
		std::vector<float  > PL_FRAME(2*params.PL_FRAME_SIZE);
		std::vector<float  > XFEC_FRAME(2*params.N_XFEC_FRAME);
		
		sink_to_matlab.pull_vector( PL_FRAME );

		PL_FRAME.erase(PL_FRAME.begin(), PL_FRAME.begin() + 2*params.M); // erase the PLHEADER

		for( int i = 1; i < params.N_PILOTS+1; i++)
		{
			PL_FRAME.erase(PL_FRAME.begin()+(i*90*16*2), PL_FRAME.begin()+(i*90*16*2)+(36*2) );
		}
		XFEC_FRAME = PL_FRAME;

		float moment2 = 0, moment4 = 0;

		for (int i = 0; i < params.N_XFEC_FRAME; i++)
		{
			float tmp = XFEC_FRAME[2*i]*XFEC_FRAME[2*i] + XFEC_FRAME[2*i+1]*XFEC_FRAME[2*i+1];
			moment2 += tmp;
			moment4 += tmp*tmp;
		}
		moment2 /= params.N_XFEC_FRAME;
		moment4 /= params.N_XFEC_FRAME;
		//std::cout << "mom2=" << moment2 << std::endl;
		//std::cout << "mom4=" << moment4 << std::endl;

		float Se = sqrt( std::abs(2 * moment2 * moment2 - moment4 ) );
		float Ne = std::abs( moment2 - Se );
		float SNR_est = 10 * log10(Se / Ne);

		//std::cout << "SNR = " << SNR_est << std::endl;

		sink_to_matlab.push_vector( XFEC_FRAME , true);

	}
	else if (sink_to_matlab.destination_chain_name == "decoding")
	{

		const std::vector<int > BCH_gen_poly{1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1};

		// buffers to store the data
		std::vector<int  > scrambler_in(params.K_BCH);	
		std::vector<int  > BCH_encoded(params.N_BCH);
		std::vector<float  > LDPC_encoded(params.N_LDPC);
		std::vector<int  > parity(params.N_BCH-params.K_BCH);
		std::vector<int  > msg(params.K_BCH);

		// Base Band scrambler
		BB_scrambler             my_scrambler;
		////////////////////////////////////////////////////
		// retrieve data from Matlab
		////////////////////////////////////////////////////
		sink_to_matlab.pull_vector( LDPC_encoded );
		//sink_to_matlab.pull_vector( BCH_encoded );

		auto dvbs2 = tools::build_dvbs2(params.K_LDPC, params.N_LDPC);

		tools::Sparse_matrix H_dvbs2;
		H_dvbs2 = build_H(*dvbs2);

		std::vector<uint32_t> info_bits_pos(params.K_LDPC);

		for(int i = 0; i< params.K_LDPC; i++)
			info_bits_pos[i] = i;//+params.N_LDPC-params.K_LDPC;

		module::Decoder_LDPC_BP_horizontal_layered_ONMS_inter<int, float> LDPC_decoder(params.K_LDPC, params.N_LDPC, 20, H_dvbs2, info_bits_pos);
		//Decoder_LDPC_BP_flooding_SPA<int, float> LDPC_decoder(params.K_LDPC, params.N_LDPC, 20, H_dvbs2, info_bits_pos, false, 1);

		std::vector<int  > LDPC_cw(params.N_LDPC);
		LDPC_decoder.decode_siho_cw(LDPC_encoded, LDPC_cw);
		
		for(int i = 0; i< params.N_BCH; i++)
			BCH_encoded[i] = LDPC_cw[i];

		// BCH decoding

		tools::BCH_polynomial_generator<int  > poly_gen(16383, 12);
		poly_gen.set_g(BCH_gen_poly);
		module::Decoder_BCH_std<int> BCH_decoder(params.K_BCH, params.N_BCH, poly_gen);

		parity.assign(BCH_encoded.begin()+params.K_BCH, BCH_encoded.begin()+params.N_BCH); // retrieve parity
		std::reverse(parity.begin(), parity.end()); // revert parity bits

		msg.assign(BCH_encoded.begin(), BCH_encoded.begin()+params.K_BCH); // retrieve message
		std::reverse(msg.begin(), msg.end()); // revert msg bits

		BCH_encoded.insert(BCH_encoded.begin(), parity.begin(), parity.end());
		BCH_encoded.insert(BCH_encoded.begin()+(params.N_BCH-params.K_BCH), msg.begin(), msg.end());

		BCH_encoded.erase(BCH_encoded.begin()+params.N_BCH, BCH_encoded.end());

		BCH_decoder.decode_hiho(BCH_encoded, scrambler_in);

		std::reverse(scrambler_in.begin(), scrambler_in.end());

		// BB descrambling
		my_scrambler.scramble(scrambler_in);

		sink_to_matlab.push_vector( scrambler_in , false);
		//sink_to_matlab.push_vector( LDPC_cw , false);

	}

	
	else
	{
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, "Invalid destination name.");
	}

	return 0;
}
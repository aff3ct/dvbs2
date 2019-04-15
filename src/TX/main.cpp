#include <vector>
#include <iostream>

#include <aff3ct.hpp>
using namespace aff3ct::module;

#include "Sink.hpp"
#include "BB_scrambler.hpp"
#include "../common/PL_scrambler/PL_scrambler.hpp"
#include "../common/Framer/Framer.hpp"
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
	const int BPS = 2; // QPSK
	const int M = 90; // number of symbols per slot
	const int P = 36; // number of symbols per pilot

	const int N_XFEC_FRAME = N_LDPC / BPS; // number of complex symbols
	const int N_PILOTS = N_XFEC_FRAME / (16*M);
	const int S = N_XFEC_FRAME / 90; // number of slots
	const int PL_FRAME_SIZE = M*(S+1) + (N_PILOTS*P);
		

	if (sink_to_matlab.destination_chain_name == "coding")
	{
		
		const std::vector<int > BCH_gen_poly{1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1};

		// buffers to store the data
		std::vector<int  > scrambler_in(K_BCH);
		std::vector<int  > bch_enc_in(K_BCH);
		std::vector<int  > bch_encoded(N_BCH);
		std::vector<int  > ldpc_encoded(N_LDPC);
		std::vector<int  > parity(N_BCH-K_BCH);
		std::vector<int  > msg(K_BCH);
		
		std::vector <float > XFEC_frame(2*N_LDPC/BPS);
		
		// Tracer
		tools::Frame_trace<>     tracer            (200, 5, std::cout);

		////////////////////////////////////////////////////
		// Create modules and blocks
		////////////////////////////////////////////////////

		// Base Band scrambler
		BB_scrambler             my_scrambler;

		// BCH encoder
		tools::BCH_polynomial_generator<int  > poly_gen(16383, 12);
		poly_gen.set_g(BCH_gen_poly);
		module::Encoder_BCH<int > BCH_encoder(K_BCH, N_BCH, poly_gen, 1);

		// LDPC encoder
		auto dvbs2 = tools::build_dvbs2(N_BCH, N_LDPC);
		module::Encoder_LDPC_DVBS2<int > LDPC_encoder(*dvbs2);

		// Modulator
		//std::unique_ptr<tools::Constellation<R>> cstl(new tools::Constellation_user <R > ("));
		std::unique_ptr<tools::Constellation<R>> cstl(new tools::Constellation_user<R>("../conf/4QAM_GRAY.mod"));
		//std::unique_ptr<tools::Constellation<R>> cstl(new tools::Constellation_PSK <R > (2));
		module::Modem_generic<> modulator(N_LDPC, std::move(cstl), tools::Sigma<R >(1.0), false, 1);

		////////////////////////////////////////////////////
		// retrieve data from Matlab
		////////////////////////////////////////////////////
		sink_to_matlab.pull_vector( scrambler_in );

		////////////////////////////////////////////////////
		// Base Band scrambling
		////////////////////////////////////////////////////
		my_scrambler.scramble( scrambler_in );

		// reverse message for Matlab compliance
		std::reverse(scrambler_in.begin(), scrambler_in.end());

		////////////////////////////////////////////////////
		// BCH Encoding
		////////////////////////////////////////////////////
		BCH_encoder.encode(scrambler_in, bch_encoded);

		// reverse parity and msg for Matlab compliance
		parity.assign(bch_encoded.begin(), bch_encoded.begin()+(N_BCH-K_BCH)); // retrieve parity
		std::reverse(parity.begin(), parity.end()); // revert parity bits
		msg.assign(bch_encoded.begin()+(N_BCH-K_BCH), bch_encoded.end()); // retrieve message
		std::reverse(msg.begin(), msg.end()); // revert msg bits

		// swap parity and msg for Matlab compliance
		bch_encoded.insert(bch_encoded.begin(), msg.begin(), msg.end());
		bch_encoded.insert(bch_encoded.begin()+K_BCH, parity.begin(), parity.end());

		bch_encoded.erase(bch_encoded.begin()+N_BCH, bch_encoded.end());

		////////////////////////////////////////////////////
		// LDPC encoding
		////////////////////////////////////////////////////
		LDPC_encoder.encode(bch_encoded, ldpc_encoded);


		//auto dvbs2 = tools::build_dvbs2(K_LDPC, N_LDPC);
/*		int K_LDPC = N_BCH;
		tools::Sparse_matrix H_dvbs2;
		H_dvbs2 = build_H(*dvbs2);

		std::vector<uint32_t> info_bits_pos(K_LDPC);

		for(int i = 0; i< K_LDPC; i++)
			info_bits_pos[i] = i;//+N_LDPC-K_LDPC;

		Decoder_LDPC_BP_horizontal_layered_ONMS_inter<int, float> LDPC_decoder(K_LDPC, N_LDPC, 200, H_dvbs2, info_bits_pos);
		//Decoder_LDPC_BP_flooding_SPA<int, float> LDPC_decoder(K_LDPC, N_LDPC, 20, H_dvbs2, info_bits_pos, false, 1);

		std::vector<float  > ldpc_encoded_llr(N_LDPC);
		std::vector<int  > BCH_encoded(N_BCH);
		std::vector<int  > ldpc_out(N_LDPC);

		for(int i = 0; i< N_LDPC; i++)
			ldpc_encoded_llr[i] = 1-2*ldpc_encoded[i];

		for(int i = 0; i< 50; i++)
			ldpc_encoded_llr[i] = -1*ldpc_encoded_llr[i];


		LDPC_decoder.decode_siho_cw(ldpc_encoded_llr, ldpc_out);



		sink_to_matlab.push_vector( ldpc_out , false);
*/
		////////////////////////////////////////////////////
		// Modulation
		////////////////////////////////////////////////////
		modulator.modulate(ldpc_encoded, XFEC_frame);

		////////////////////////////////////////////////////
		// PL_HEADER generation : SOF + PLS code
		////////////////////////////////////////////////////

		std::vector<float> PL_FRAME(2*PL_FRAME_SIZE);

		Framer<float> DVBS2_framer(2*N_LDPC/BPS, 2*PL_FRAME_SIZE);

		DVBS2_framer.generate(XFEC_frame, PL_FRAME);
				
		////////////////////////////////////////////////////
		// PL Scrambling
		////////////////////////////////////////////////////

		std::vector<float> SCRAMBLED_PL_FRAME(2*PL_FRAME_SIZE-2*M);

		SCRAMBLED_PL_FRAME.insert(SCRAMBLED_PL_FRAME.begin(), PL_FRAME.begin(), PL_FRAME.begin()+2*M);

		PL_scrambler<float> complex_scrambler(2*PL_FRAME_SIZE, M, true); 

		complex_scrambler.scramble(PL_FRAME, SCRAMBLED_PL_FRAME);

		////////////////////////////////////////////////////
		// SHAPING
		////////////////////////////////////////////////////

		const float ROLLOFF = 0.05;
		const int N_SYMBOLS = 8370;
		const int OSF       = 4;
		const int GRP_DELAY = 50;
		Filter_UPRRC_ccr_naive<float> shaping_filter((N_SYMBOLS + GRP_DELAY)*2, ROLLOFF, OSF, GRP_DELAY);

		std::vector<float  > shaping_in (N_SYMBOLS*2, 0.0f);
		std::vector<float  > shaping_out((N_SYMBOLS+ GRP_DELAY)*2*OSF);
		std::vector<float  > shaping_cut(N_SYMBOLS*2*OSF);

		//sink_to_matlab.pull_vector(shaping_in);
		shaping_in = SCRAMBLED_PL_FRAME;
		shaping_in.resize((N_SYMBOLS+ GRP_DELAY)*2, 0.0f);
		shaping_filter.filter(shaping_in, shaping_out);
		std::copy(shaping_out.begin()+GRP_DELAY*OSF*2, shaping_out.begin()+(GRP_DELAY+N_SYMBOLS)*OSF*2, shaping_cut.begin());
		sink_to_matlab.push_vector( shaping_cut , true);

	}
	else
	{
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, "Invalid destination name.");
	}

	return 0;
}
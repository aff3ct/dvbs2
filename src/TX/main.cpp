#include <vector>
#include <iostream>

#include <aff3ct.hpp>

#include "BB_scrambler.hpp"
#include "Framer/Framer.hpp"
#include "Filter/Filter_UPFIR/Filter_UPRRC/Filter_UPRRC_ccr_naive.hpp"
#include "PL_scrambler/PL_scrambler.hpp"
#include "Sink.hpp"
#include "DVBS2_params.hpp"

using namespace aff3ct;

int main(int argc, char** argv)
{
	auto params = DVBS2_params(argc, argv);

	Sink sink_to_matlab (params.mat2aff_file_name, params.aff2mat_file_name);

	if (sink_to_matlab.destination_chain_name == "coding")
	{
		// buffers to store the data
		std::vector<int>   scrambler_in(params.K_BCH);
		std::vector<int>   bch_enc_in(params.K_BCH);
		std::vector<int>   bch_encoded(params.N_BCH);
		std::vector<int>   ldpc_encoded(params.N_LDPC);
		std::vector<int>   parity(params.N_BCH-params.K_BCH);
		std::vector<int>   msg(params.K_BCH);
		std::vector<float> XFEC_frame(2*params.N_LDPC/params.BPS);
		
		// Tracer
		tools::Frame_trace<>     tracer            (200, 5, std::cout);

		////////////////////////////////////////////////////
		// Create modules and blocks
		////////////////////////////////////////////////////

		// Base Band scrambler
		BB_scrambler             my_scrambler;

		// BCH encoder
		tools::BCH_polynomial_generator<int  > poly_gen(16383, 12);
		poly_gen.set_g(params.BCH_gen_poly);
		module::Encoder_BCH<int > BCH_encoder(params.K_BCH, params.N_BCH, poly_gen, 1);

		// LDPC encoder
		auto dvbs2 = tools::build_dvbs2(params.N_BCH, params.N_LDPC);
		module::Encoder_LDPC_DVBS2<int > LDPC_encoder(*dvbs2);

		// Modulator
		//std::unique_ptr<tools::Constellation<R>> cstl(new tools::Constellation_user <R > ("));
		std::unique_ptr<tools::Constellation<R>> cstl(new tools::Constellation_user<R>("../conf/4QAM_GRAY.mod"));
		//std::unique_ptr<tools::Constellation<R>> cstl(new tools::Constellation_PSK <R > (2));
		module::Modem_generic<> modulator(params.N_LDPC, std::move(cstl), tools::Sigma<R >(1.0), false, 1);

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
		parity.assign(bch_encoded.begin(), bch_encoded.begin()+(params.N_BCH-params.K_BCH)); // retrieve parity
		std::reverse(parity.begin(), parity.end()); // revert parity bits
		msg.assign(bch_encoded.begin()+(params.N_BCH-params.K_BCH), bch_encoded.end()); // retrieve message
		std::reverse(msg.begin(), msg.end()); // revert msg bits

		// swap parity and msg for Matlab compliance
		bch_encoded.insert(bch_encoded.begin(), msg.begin(), msg.end());
		bch_encoded.insert(bch_encoded.begin()+params.K_BCH, parity.begin(), parity.end());

		bch_encoded.erase(bch_encoded.begin()+params.N_BCH, bch_encoded.end());

		////////////////////////////////////////////////////
		// LDPC encoding
		////////////////////////////////////////////////////
		LDPC_encoder.encode(bch_encoded, ldpc_encoded);

		//sink_to_matlab.push_vector(ldpc_encoded , false);

		////////////////////////////////////////////////////
		// Interleaver
		////////////////////////////////////////////////////

		// auto interleaver_core = tools::Interleaver_core_column_row<uint32_t>(params.N_LDPC, params.ITL_N_COLS, params.READ_ORDER);
		// auto interleaver      = module::Interleaver<>(interleaver_core);

		// std::vector<int> input (16);
		// std::vector<int> output(16);
		// std::iota(input.begin(), input.end(), 0);
		// for (auto i : input)
		// 	std::cout << i << " ";
		// std::cout << std::endl;
		// interleaver_core.init();
		// interleaver.interleave(input, output);
		// for (auto i : output)
		// 	std::cout << i << " ";
		// std::cout << std::endl;

		////////////////////////////////////////////////////
		// Modulation
		////////////////////////////////////////////////////
		modulator.modulate(ldpc_encoded, XFEC_frame);

		//sink_to_matlab.push_vector(XFEC_frame , true);
		////////////////////////////////////////////////////
		// PL_HEADER generation : SOF + PLS code
		////////////////////////////////////////////////////

		std::vector<float> pl_frame(2*params.PL_FRAME_SIZE);

		module::Framer<float> DVBS2_framer(2*params.N_LDPC/params.BPS, 2*params.PL_FRAME_SIZE, params.MODCOD);

		DVBS2_framer.generate(XFEC_frame, pl_frame);
		//sink_to_matlab.push_vector(pl_frame , true);
				
		////////////////////////////////////////////////////
		// PL Scrambling
		////////////////////////////////////////////////////

		std::vector<float> scrambled_pl_frame(2*params.PL_FRAME_SIZE-2*params.M);

		scrambled_pl_frame.insert(scrambled_pl_frame.begin(), pl_frame.begin(), pl_frame.begin()+2*params.M);

		module::PL_scrambler<float> complex_scrambler(2*params.PL_FRAME_SIZE, params.M, true); 

		complex_scrambler.scramble(pl_frame, scrambled_pl_frame);

		//sink_to_matlab.push_vector(scrambled_pl_frame , true);
		////////////////////////////////////////////////////
		// SHAPING
		////////////////////////////////////////////////////


		module::Filter_UPRRC_ccr_naive<float> shaping_filter((params.PL_FRAME_SIZE + params.GRP_DELAY) * 2,
		                                                      params.ROLLOFF,
		                                                      params.OSF,
		                                                      params.GRP_DELAY);

		std::vector<float  > shaping_in (params.PL_FRAME_SIZE * 2, 0.0f);
		std::vector<float  > shaping_out((params.PL_FRAME_SIZE + params.GRP_DELAY) * 2 * params.OSF);
		std::vector<float  > shaping_cut(params.PL_FRAME_SIZE * 2 * params.OSF);

		//sink_to_matlab.pull_vector(shaping_in);
		shaping_in = scrambled_pl_frame;
		shaping_in.resize((params.PL_FRAME_SIZE + params.GRP_DELAY) * 2, 0.0f);
		shaping_filter.filter(shaping_in, shaping_out);
		std::copy(shaping_out.begin() + params.GRP_DELAY                      * params.OSF * 2, 
		          shaping_out.begin() + (params.GRP_DELAY + params.PL_FRAME_SIZE) * params.OSF * 2,
		          shaping_cut.begin());
		sink_to_matlab.push_vector(shaping_cut , true);

	}
	else
	{
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, "Invalid destination name.");
	}

	return 0;
}
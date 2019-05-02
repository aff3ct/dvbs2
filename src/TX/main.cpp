#include <vector>
#include <iostream>

#include <aff3ct.hpp>

#include "Scrambler/Scrambler_BB/Scrambler_BB.hpp"
#include "Params_DVBS2O/Params_DVBS2O.hpp"
#include "Factory_DVBS2O/Factory_DVBS2O.hpp"
#include "Framer/Framer.hpp"
#include "Filter/Filter_UPFIR/Filter_UPRRC/Filter_UPRRC_ccr_naive.hpp"
#include "Scrambler/Scrambler_PL/Scrambler_PL.hpp"
#include "Sink/Sink.hpp"


using namespace aff3ct;

int main(int argc, char** argv)
{
	auto params = Params_DVBS2O(argc, argv);

	Sink sink_to_matlab (params.mat2aff_file_name, params.aff2mat_file_name);

	if (sink_to_matlab.destination_chain_name == "coding")
	{
		// buffers to store the data
		std::vector<int>   scrambler_in     (params.K_BCH);
		std::vector<int>   scrambler_out    (params.K_BCH);
		std::vector<int>   bch_enc_in       (params.K_BCH);
		std::vector<int>   bch_encoded      (params.N_BCH);
		std::vector<int>   ldpc_encoded     (params.N_LDPC);
		std::vector<int>   ldpc_encoded_itlv(params.N_LDPC);
		std::vector<int>   parity           (params.N_BCH - params.K_BCH);
		std::vector<int>   msg              (params.K_BCH);
		std::vector<float> XFEC_frame       (2 * params.N_LDPC / params.BPS);
		
		// Tracer
		tools::Frame_trace<>     tracer            (200, 5, std::cout);

		////////////////////////////////////////////////////
		// Create modules and blocks
		////////////////////////////////////////////////////

		// Base Band scrambler
		module::Scrambler_BB<int>             my_scrambler(params.K_BCH);

		std::unique_ptr<module::Encoder_BCH<>  > BCH_encoder (Factory_DVBS2O::build_bch_encoder<> (params));
		std::unique_ptr<module::Codec_LDPC<>   > LDPC_cdc    (Factory_DVBS2O::build_ldpc_cdc<>(params));
		std::unique_ptr<tools::Constellation<R>> cstl        (new tools::Constellation_user<R>(params.constellation_file));
		module::Modem_generic<> modulator(params.N_LDPC, std::move(cstl), tools::Sigma<R >(1.0), false, 1);

		auto& LDPC_encoder = LDPC_cdc->get_encoder();

		// Modulator

		////////////////////////////////////////////////////
		// retrieve data from Matlab
		////////////////////////////////////////////////////
		sink_to_matlab.pull_vector(scrambler_in);

		////////////////////////////////////////////////////
		// Base Band scrambling
		////////////////////////////////////////////////////
		my_scrambler.scramble(scrambler_in, scrambler_out);


		////////////////////////////////////////////////////
		// BCH Encoding
		////////////////////////////////////////////////////
		// reverse message for aff3ct BCH compliance
		std::reverse(scrambler_out.begin(), scrambler_out.end());
		BCH_encoder->encode(scrambler_out, bch_encoded);
		std::reverse(bch_encoded.begin(), bch_encoded.end());

		////////////////////////////////////////////////////
		// LDPC encoding
		////////////////////////////////////////////////////
		(*LDPC_encoder).encode(bch_encoded, ldpc_encoded);

		//sink_to_matlab.push_vector(ldpc_encoded , false);

		////////////////////////////////////////////////////
		// Interleaver
		////////////////////////////////////////////////////

		if (params.MODCOD == "QPSK-S_8/9" || params.MODCOD == "QPSK-S_3/5" || params.MODCOD == "")
		{
			ldpc_encoded_itlv = ldpc_encoded;
		}
		else if (params.MODCOD == "8PSK-S_8/9" || params.MODCOD == "8PSK-S_3/5" || params.MODCOD == "16APSK-S_8/9")
		{
			auto interleaver_core = tools::Interleaver_core_column_row<uint32_t>(params.N_LDPC, params.ITL_N_COLS, params.READ_ORDER);
			auto interleaver      = module::Interleaver<>(interleaver_core);
			interleaver_core.init();
			interleaver.interleave(ldpc_encoded, ldpc_encoded_itlv);
		}
		else
			throw tools::invalid_argument(__FILE__, __LINE__, __func__, params.MODCOD + " mod-cod scheme not yet supported.");
				
		//sink_to_matlab.push_vector(ldpc_encoded_itlv , false);

		////////////////////////////////////////////////////
		// Modulation
		////////////////////////////////////////////////////
		modulator.modulate(ldpc_encoded_itlv, XFEC_frame);

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

		module::Scrambler_PL<float> complex_scrambler(2*params.PL_FRAME_SIZE, params.M); 

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
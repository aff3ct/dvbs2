#include <vector>
#include <iostream>

#include <aff3ct.hpp>

#include "Sink.hpp"
#include "BB_scrambler.hpp"
#include "../common/PL_scrambler/PL_scrambler.hpp"
#include "Filter/Filter_UPFIR/Filter_UPRRC/Filter_UPRRC_ccr_naive.hpp"
#include "DVBS2_params.hpp"

using namespace aff3ct;
using Decoder_LDPC_DVBS2 = module::Decoder_LDPC_BP_horizontal_layered_ONMS_inter<int, float>;

int main(int argc, char** argv)
{
	auto params = DVBS2_params(argc, argv);

	Sink sink_to_matlab    (params.mat2aff_file_name, params.aff2mat_file_name);

	tools::Frame_trace<>     tracer            (20, 5, std::cout);

	BB_scrambler             my_scrambler;	

	module::PL_scrambler<float> complex_scrambler(2*params.PL_FRAME_SIZE, params.M, false);
	auto H_dvbs2 = build_H(*tools::build_dvbs2(params.K_LDPC, params.N_LDPC));
	std::vector<uint32_t> info_bits_pos(params.K_LDPC);
	std::iota(info_bits_pos.begin(), info_bits_pos.end(), 0);
	Decoder_LDPC_DVBS2 LDPC_decoder(params.K_LDPC, params.N_LDPC, 200, H_dvbs2, info_bits_pos);
	tools::BCH_polynomial_generator<int> poly_gen(params.N_BCH_unshortened, 12);
	poly_gen.set_g(params.BCH_gen_poly);
	module::Decoder_BCH_std<int> BCH_decoder(params.K_BCH, params.N_BCH, poly_gen);

	std::vector<float> scrambled_pl_frame (2 * params.PL_FRAME_SIZE);
	std::vector<float> pl_frame_scr       (2 * params.PL_FRAME_SIZE - 2 * params.M);
	std::vector<float> pl_frame_defr      (2 * params.PL_FRAME_SIZE);
	std::vector<float> xfec_frame         (2 * params.N_XFEC_FRAME);
	std::vector<int>   scrambler_in       (params.K_BCH);	
	std::vector<int>   BCH_encoded        (params.N_BCH);
	std::vector<float> LDPC_encoded       (params.N_LDPC);
	std::vector<float> LDPC_encoded_itlv  (params.N_LDPC);
	std::vector<int>   parity             (params.N_BCH - params.K_BCH);
	std::vector<int>   msg                (params.K_BCH);
	std::vector<int>   LDPC_cw            (params.N_LDPC);
	std::vector<float> H_vec              (2 * params.N_XFEC_FRAME);

	if (sink_to_matlab.destination_chain_name == "descramble")
	{
		sink_to_matlab.pull_vector( scrambled_pl_frame );

		pl_frame_scr.insert(pl_frame_scr.begin(), scrambled_pl_frame.begin(), scrambled_pl_frame.begin()+ 2 * params.M);
		complex_scrambler.scramble(scrambled_pl_frame, pl_frame_scr);

		sink_to_matlab.push_vector( pl_frame_scr , true);
	}
	else if (sink_to_matlab.destination_chain_name == "demod_decod")
	{
		sink_to_matlab.pull_vector( pl_frame_defr );

		pl_frame_defr.erase(pl_frame_defr.begin(), pl_frame_defr.begin() + 2 * params.M); // erase the PLHEADER

		for( int i = 1; i < params.N_PILOTS+1; i++)
		{
			pl_frame_defr.erase(pl_frame_defr.begin()+(i * params.M * 16 * 2),
			                    pl_frame_defr.begin()+(i * params.M * 16 * 2) + (params.P * 2));
		}
		xfec_frame = pl_frame_defr;

		//sink_to_matlab.push_vector( xfec_frame , true);

		////////////////////////////////////////////////////
		// Channel estimation
		////////////////////////////////////////////////////

		float moment2 = 0, moment4 = 0;
		float pow_tot, pow_sig_util, sigma_n2;

		for (int i = 0; i < params.N_XFEC_FRAME; i++)
		{
			float tmp = xfec_frame[2 * i]*xfec_frame[2 * i] + xfec_frame[2 * i + 1]*xfec_frame[2 * i + 1];
			moment2 += tmp;
			moment4 += tmp*tmp;
		}
		moment2 /= params.N_XFEC_FRAME;
		moment4 /= params.N_XFEC_FRAME;
		//std::cout << "mom2=" << moment2 << std::endl;
		//std::cout << "mom4=" << moment4 << std::endl;

		float Se      = sqrt( std::abs(2 * moment2 * moment2 - moment4 ) );
		float Ne      = std::abs( moment2 - Se );
		float SNR_est = 10 * log10(Se / Ne);
SNR_est = 15.8;

		pow_tot = moment2;

		pow_sig_util = pow_tot / (1+(pow(10, (-1 * SNR_est/10))));
		sigma_n2 = pow_tot - pow_sig_util;

		float H = sqrt(pow_sig_util);

		for (int i = 0; i < params.N_XFEC_FRAME; i++)
		{
			H_vec[2*i] = H;
			H_vec[2*i+1] = 0;
		}

		////////////////////////////////////////////////////
		// Soft demodulation
		////////////////////////////////////////////////////

		std::string constellation_file;

		if (params.MODCOD == "QPSK-S_8/9" || params.MODCOD == "QPSK-S_3/5" || params.MODCOD == "")
		{
			constellation_file = "../conf/4QAM_GRAY.mod";
		}
		else if (params.MODCOD == "8PSK-S_8/9" || params.MODCOD == "8PSK-S_3/5")
		{
			constellation_file = "../conf/8PSK.mod";
		}
		else if (params.MODCOD == "16APSK-S_8/9")
		{
			//constellation_file = "../conf/4QAM_GRAY.mod";
			throw tools::invalid_argument(__FILE__, __LINE__, __func__, params.MODCOD + " mod-cod scheme not yet supported.");
		}
		else
			throw tools::invalid_argument(__FILE__, __LINE__, __func__, params.MODCOD + " mod-cod scheme not yet supported.");
				
		//std::unique_ptr<tools::Constellation<R>> cstl(new tools::Constellation_user<R>("../conf/4QAM_GRAY.mod"));
		std::unique_ptr<tools::Constellation<R>> cstl(new tools::Constellation_user<R>(constellation_file));

		module::Modem_generic<int,float,float,tools::max_star<float>> modulator(params.N_LDPC, std::move(cstl));
		modulator.set_noise(tools::Sigma<float>(sqrt(sigma_n2/2), 0, 0));
		std::vector<float  > LDPC_encoded(params.N_LDPC);

		modulator.demodulate_wg(H_vec, xfec_frame, LDPC_encoded_itlv, 1);
		//sink_to_matlab.push_vector( LDPC_encoded_itlv , false);

		if (params.MODCOD == "QPSK-S_8/9" || params.MODCOD == "QPSK-S_3/5" || params.MODCOD == "")
		{
			LDPC_encoded = LDPC_encoded_itlv;
		}
		else if (params.MODCOD == "8PSK-S_8/9" || params.MODCOD == "8PSK-S_3/5" || params.MODCOD == "16APSK-S_8/9")
		{
			auto interleaver_core = tools::Interleaver_core_column_row<uint32_t>(params.N_LDPC, params.ITL_N_COLS, params.READ_ORDER);
			auto interleaver      = module::Interleaver<float>(interleaver_core);
			interleaver_core.init();
			interleaver.deinterleave(LDPC_encoded_itlv, LDPC_encoded);
		}
		else
			throw tools::invalid_argument(__FILE__, __LINE__, __func__, params.MODCOD + " mod-cod scheme not yet supported.");

		//sink_to_matlab.push_vector( LDPC_encoded , false);

		////////////////////////////////////////////////////
		// LDPC decoding
		////////////////////////////////////////////////////
		
		LDPC_decoder.decode_siho_cw(LDPC_encoded, LDPC_cw);
		std::copy(LDPC_cw.begin(), LDPC_cw.begin() + params.N_BCH, BCH_encoded.begin());

		//sink_to_matlab.push_vector( BCH_encoded , false);

		////////////////////////////////////////////////////
		// BCH decoding
		////////////////////////////////////////////////////

		parity.assign(BCH_encoded.begin() + params.K_BCH, BCH_encoded.begin() + params.N_BCH); // retrieve parity
		std::reverse(parity.begin(), parity.end());                                            // revert parity bits
		msg.assign(BCH_encoded.begin(), BCH_encoded.begin() + params.K_BCH);                   // retrieve message
		std::reverse(msg.begin(), msg.end());                                                  // revert msg bits
		BCH_encoded.insert(BCH_encoded.begin(), parity.begin(), parity.end());
		BCH_encoded.insert(BCH_encoded.begin() + (params.N_BCH - params.K_BCH), msg.begin(), msg.end());
		BCH_encoded.erase(BCH_encoded.begin() + params.N_BCH, BCH_encoded.end());
		BCH_decoder.decode_hiho(BCH_encoded, scrambler_in);
		std::reverse(scrambler_in.begin(), scrambler_in.end());

		//sink_to_matlab.push_vector( scrambler_in , false);
		////////////////////////////////////////////////////
		// BB descrambling
		////////////////////////////////////////////////////

		my_scrambler.scramble(scrambler_in);

		sink_to_matlab.push_vector( scrambler_in , false);
	}

	
	else
	{
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, "Invalid destination name.");
	}

	return 0;
}
#include <vector>
#include <iostream>

#include <aff3ct.hpp>

#include "Scrambler/Scrambler_BB/Scrambler_BB.hpp"
#include "Scrambler/Scrambler_PL/Scrambler_PL.hpp"
#include "Params_DVBS2O/Params_DVBS2O.hpp"
#include "Framer/Framer.hpp"
#include "Filter/Filter_UPFIR/Filter_UPRRC/Filter_UPRRC_ccr_naive.hpp"
#include "Sink/Sink.hpp"
#include "Factory_DVBS2O/Factory_DVBS2O.hpp"
#include "Estimator/Estimator.hpp"

using namespace aff3ct;

int main(int argc, char** argv)
{
	auto params = Params_DVBS2O(argc, argv);

	tools::BCH_polynomial_generator<B> poly_gen (params.N_BCH_unshortened, 12);
	std::unique_ptr<tools::Constellation<R>> cstl(new tools::Constellation_user<R>(params.constellation_file));

	Sink sink_to_matlab    (params.mat2aff_file_name, params.aff2mat_file_name);
	std::unique_ptr<module::Scrambler       <>> bb_scrambler  (Factory_DVBS2O::build_bb_scrambler<> (params                         ));
	std::unique_ptr<module::Scrambler  <float>> pl_scrambler  (Factory_DVBS2O::build_pl_scrambler<> (params                         ));
	std::unique_ptr<module::Codec_LDPC      <>> LDPC_cdc      (Factory_DVBS2O::build_ldpc_cdc    <> (params                         ));
	std::unique_ptr<module::Decoder_BCH_std <>> BCH_decoder   (Factory_DVBS2O::build_bch_decoder <> (params, poly_gen               ));
	std::unique_ptr<module::Modem           <>> modulator     (Factory_DVBS2O::build_modem       <> (params, std::move(cstl)        ));
	std::unique_ptr<tools ::Interleaver_core<>> itl_core      (Factory_DVBS2O::build_itl_core    <> (params                         ));
	std::unique_ptr<module::Interleaver<float,uint32_t>> itl  (Factory_DVBS2O::build_itl         <float,uint32_t> (params, *itl_core));
	std::unique_ptr<module::Estimator       <>> estimator     (Factory_DVBS2O::build_estimator   <> (params                         ));
	std::unique_ptr<module::Framer          <>> framer        (Factory_DVBS2O::build_framer      <> (params                 ));
	auto& LDPC_decoder    = LDPC_cdc->get_decoder_siho();

	std::vector<float> scrambled_pl_frame (2 * params.PL_FRAME_SIZE);
	std::vector<float> pl_frame_scr       (2 * params.PL_FRAME_SIZE - 2 * params.M);
	std::vector<float> pl_frame_defr      (2 * params.PL_FRAME_SIZE);
	std::vector<float> xfec_frame         (2 * params.N_XFEC_FRAME);
	std::vector<int>   scrambler_in       (params.K_BCH);	
	std::vector<int>   scrambler_out      (params.K_BCH);	
	std::vector<int>   BCH_encoded        (params.N_BCH);
	std::vector<float> LDPC_encoded       (params.N_LDPC);
	std::vector<float> LDPC_encoded_itlv  (params.N_LDPC);
	std::vector<int>   parity             (params.N_BCH - params.K_BCH);
	std::vector<int>   msg                (params.K_BCH);
	std::vector<int>   LDPC_cw            (params.N_LDPC);
	std::vector<float> H_vec              (2 * params.N_XFEC_FRAME);

	poly_gen.set_g(params.BCH_gen_poly);
	itl_core->init();


	sink_to_matlab.pull_vector( pl_frame_defr );
	framer      ->remove_plh(pl_frame_defr, xfec_frame);
	estimator   ->estimate(xfec_frame, H_vec);
	modulator   ->set_noise(tools::Sigma<float>(sqrt(estimator->get_sigma_n2()/2), 0, 0));
	modulator   ->demodulate_wg(H_vec, xfec_frame, LDPC_encoded_itlv, 1);
	itl         ->deinterleave(LDPC_encoded_itlv, LDPC_encoded);
	LDPC_decoder->decode_siho(LDPC_encoded, BCH_encoded);
	BCH_decoder ->decode_hiho(BCH_encoded, scrambler_in);
	bb_scrambler->scramble(scrambler_in, scrambler_out);
	sink_to_matlab.push_vector(scrambler_out , false);

	return 0;
}

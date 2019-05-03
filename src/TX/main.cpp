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

	tools::Frame_trace<>     tracer            (200, 5, std::cout);

	// buffers to store the data
	std::vector<int>   scrambler_in      (params.K_BCH);
	std::vector<int>   scrambler_out     (params.K_BCH);
	std::vector<int>   bch_enc_in        (params.K_BCH);
	std::vector<int>   bch_encoded       (params.N_BCH);
	std::vector<int>   ldpc_encoded      (params.N_LDPC);
	std::vector<int>   ldpc_encoded_itlv (params.N_LDPC);
	std::vector<int>   parity            (params.N_BCH - params.K_BCH);
	std::vector<int>   msg               (params.K_BCH);
	std::vector<float> XFEC_frame        (2 * params.N_LDPC / params.BPS);
	std::vector<float> pl_frame          (2 * params.PL_FRAME_SIZE);
	std::vector<float> scrambled_pl_frame(2 * params.PL_FRAME_SIZE);
	std::vector<float> shaping_in        ((params.PL_FRAME_SIZE + params.GRP_DELAY) * 2, 0.0f);
	std::vector<float> shaping_out       ((params.PL_FRAME_SIZE + params.GRP_DELAY) * 2 * params.OSF);
	std::vector<float> shaping_cut       (params.PL_FRAME_SIZE * 2 * params.OSF);

	// construct modules
	tools::BCH_polynomial_generator<B> poly_gen (params.N_BCH_unshortened, 12);
	std::unique_ptr<tools::Constellation<R>> cstl(new tools::Constellation_user<R>(params.constellation_file));

	Sink                                        sink_to_matlab(params.mat2aff_file_name, params.aff2mat_file_name);
	std::unique_ptr<module::Scrambler_BB    <>> bb_scrambler  (Factory_DVBS2O::build_bb_scrambler<> (params                 ));
	std::unique_ptr<module::Encoder_BCH     <>> BCH_encoder   (Factory_DVBS2O::build_bch_encoder <> (params, poly_gen       ));
	std::unique_ptr<module::Codec_LDPC      <>> LDPC_cdc      (Factory_DVBS2O::build_ldpc_cdc    <> (params                 ));
	std::unique_ptr<tools ::Interleaver_core<>> itl_core      (Factory_DVBS2O::build_itl_core    <> (params                 ));
	std::unique_ptr<module::Interleaver     <>> itl           (Factory_DVBS2O::build_itl         <> (params, *itl_core      ));
	std::unique_ptr<module::Modem_generic   <>> modulator     (Factory_DVBS2O::build_modem       <> (params, std::move(cstl)));
	std::unique_ptr<module::Framer          <>> framer        (Factory_DVBS2O::build_framer      <> (params                 ));
	std::unique_ptr<module::Scrambler_PL    <>> pl_scrambler  (Factory_DVBS2O::build_pl_scrambler<> (params                 ));
	module::Filter_UPRRC_ccr_naive<float>       shaping_filter((params.PL_FRAME_SIZE + params.GRP_DELAY) * 2,
	                                                            params.ROLLOFF,
	                                                            params.OSF,
	                                                            params.GRP_DELAY);
	
	auto& LDPC_encoder    = LDPC_cdc->get_encoder();

	// initialization
	poly_gen.set_g(params.BCH_gen_poly);
	itl_core->init();

	// execution
	sink_to_matlab.pull_vector(scrambler_in);
	bb_scrambler->scramble    (scrambler_in, scrambler_out);
	std::reverse              (scrambler_out.begin(), scrambler_out.end());// reverse message for aff3ct BCH compliance
	BCH_encoder ->encode      (scrambler_out, bch_encoded);
	std::reverse              (bch_encoded.begin(), bch_encoded.end());
	LDPC_encoder->encode      (bch_encoded, ldpc_encoded);
	itl         ->interleave  (ldpc_encoded, ldpc_encoded_itlv);
	modulator   ->modulate    (ldpc_encoded_itlv, XFEC_frame);
	framer      ->generate    (XFEC_frame, pl_frame);
	
	std::copy(pl_frame.begin(), pl_frame.begin() + 2 * params.M, scrambled_pl_frame.begin());
	pl_scrambler->scramble(pl_frame, scrambled_pl_frame);

	std::copy(scrambled_pl_frame.begin(), scrambled_pl_frame.end(), shaping_in.begin());
	shaping_filter.filter     (shaping_in, shaping_out);
	std::copy(shaping_out.begin() + (params.GRP_DELAY                       ) * params.OSF * 2, 
	          shaping_out.begin() + (params.GRP_DELAY + params.PL_FRAME_SIZE) * params.OSF * 2,
	          shaping_cut.begin());
	sink_to_matlab.push_vector(shaping_cut , true);

	return 0;
}
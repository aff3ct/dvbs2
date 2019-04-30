#include "Factory_DVBS2O.hpp"

template <typename B>
module::Encoder_BCH<B>* Factory_DVBS2O
::build_bch_encoder(Params_DVBS2O& params) 
{
	// TODO : fix memory leak by putting the generator in a struct or class, unique_ptr ?

	// std::unique_ptr<tools::BCH_polynomial_generator<B>> poly_gen (new tools::BCH_polynomial_generator<B> (16383, 12);)
	auto * poly_gen = new tools::BCH_polynomial_generator<B> (params.N_BCH_unshortened, 12);
	poly_gen->set_g(params.BCH_gen_poly);
	return new module::Encoder_BCH<B>(params.K_BCH, params.N_BCH, *poly_gen, 1);
}

template <typename B,typename Q>
module::Codec_LDPC<B,Q>* Factory_DVBS2O
::build_ldpc_cdc(Params_DVBS2O& params) 
{
	factory::Codec_LDPC::parameters p_cdc;

	// store parameters
	p_cdc.enc->type     = "LDPC_DVBS2";
	p_cdc.enc->N_cw     = params.N_LDPC;
	p_cdc.enc->K        = params.N_BCH;
	p_cdc.dec->N_cw     = p_cdc.enc->N_cw;
	p_cdc.dec->K        = p_cdc.enc->K;
	p_cdc.dec->n_frames = p_cdc.enc->n_frames;
	p_cdc.enc->R        = (float)p_cdc.enc->K / (float)p_cdc.enc->N_cw;
	p_cdc.dec->R        = (float)p_cdc.dec->K / (float)p_cdc.dec->N_cw;
	p_cdc.K             = p_cdc.enc->K;
	p_cdc.N_cw          = p_cdc.enc->N_cw;
	p_cdc.N             = p_cdc.N_cw;
	// build ldpc codec
	return p_cdc.build();
}


template aff3ct::module::Encoder_BCH<B>*        Factory_DVBS2O::build_bch_encoder<B> (Params_DVBS2O& params);
template aff3ct::module::Codec_LDPC<B,Q>*       Factory_DVBS2O::build_ldpc_cdc<B,Q>  (Params_DVBS2O& params);

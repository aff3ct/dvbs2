#ifndef FACTORY_DVBS2O_HPP
#define FACTORY_DVBS2O_HPP

#include <aff3ct.hpp>

#include "../Params_DVBS2O/Params_DVBS2O.hpp"

struct Factory_DVBS2O {
	template <typename B = int>
	static module::Encoder_BCH<B>* build_bch_encoder (Params_DVBS2O& params);

	template <typename B = int, typename Q = float>
	static module::Codec_LDPC<B,Q>* build_ldpc_cdc (Params_DVBS2O& params);
};

#endif /* FACTORY_DVBS2O_HPP */

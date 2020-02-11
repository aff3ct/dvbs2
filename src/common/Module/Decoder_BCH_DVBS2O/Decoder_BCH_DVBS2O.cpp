#include <vector>
#include <cmath>
#include <iostream>

#include "Module/Decoder_BCH_DVBS2O/Decoder_BCH_DVBS2O.hpp"

using namespace aff3ct;
using namespace aff3ct::module;

template <typename B, typename R>
Decoder_BCH_DVBS2O<B, R>
::Decoder_BCH_DVBS2O(const int& K, const int& N, const tools::BCH_polynomial_generator<B>& GF_poly, const int n_frames)
: Decoder_BCH_std<B, R>(K, N, GF_poly, n_frames)
{
	const std::string name = "Decoder_BCH_DVBS2O";
	this->set_name(name);
}

template <typename B, typename R>
Decoder_BCH_DVBS2O<B,R>* Decoder_BCH_DVBS2O<B,R>
::clone() const
{
	auto m = new Decoder_BCH_DVBS2O(*this);
	m->deep_copy(*this);
	return m;
}

template <typename B, typename R>
int Decoder_BCH_DVBS2O<B, R>
::_decode_hiho(const B *Y_N, B *V_K, const int frame_id)
{
	std::reverse_copy(Y_N, Y_N + this->N, this->YH_N.begin());

	auto status = this->_decode(this->YH_N.data(), frame_id);

	std::reverse_copy(this->YH_N.data() + this->N - this->K, this->YH_N.data() + this->N, V_K);

	return status;
}

template <typename B, typename R>
int Decoder_BCH_DVBS2O<B, R>
::_decode_hiho_cw(const B *Y_N, B *V_N, const int frame_id)
{
	throw tools::unimplemented_error(__FILE__, __LINE__, __func__);
}

template <typename B, typename R>
int Decoder_BCH_DVBS2O<B, R>
::_decode_siho(const R *Y_N, B *V_K, const int frame_id)
{
	throw tools::unimplemented_error(__FILE__, __LINE__, __func__);
}

template <typename B, typename R>
int Decoder_BCH_DVBS2O<B, R>
::_decode_siho_cw(const R *Y_N, B *V_N, const int frame_id)
{
	throw tools::unimplemented_error(__FILE__, __LINE__, __func__);
}


// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
#ifdef AFF3CT_MULTI_PREC
template class aff3ct::module::Decoder_BCH_DVBS2O<B_8,Q_8>;
template class aff3ct::module::Decoder_BCH_DVBS2O<B_16,Q_16>;
template class aff3ct::module::Decoder_BCH_DVBS2O<B_32,Q_32>;
template class aff3ct::module::Decoder_BCH_DVBS2O<B_64,Q_64>;
#else
template class aff3ct::module::Decoder_BCH_DVBS2O<B,Q>;
#endif
// ==================================================================================== explicit template instantiation

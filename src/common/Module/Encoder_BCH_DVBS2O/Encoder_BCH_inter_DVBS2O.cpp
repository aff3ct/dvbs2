#include <vector>
#include <cmath>
#include <iostream>

#include "Module/Encoder_BCH_DVBS2O/Encoder_BCH_inter_DVBS2O.hpp"

using namespace aff3ct;
using namespace aff3ct::module;

template <typename B>
Encoder_BCH_inter_DVBS2O<B>
::Encoder_BCH_inter_DVBS2O(const int& K, const int& N, const tools::BCH_polynomial_generator<B>& GF_poly, const int n_frames)
: Encoder_BCH_inter<B>(K, N, GF_poly, n_frames),
  U_K_rev(K * this->simd_inter_frame_level)
{
	const std::string name = "Encoder_BCH_inter_DVBS2O";
	this->set_name(name);
}

template <typename B>
Encoder_BCH_inter_DVBS2O<B>* Encoder_BCH_inter_DVBS2O<B>
::clone() const
{
	auto m = new Encoder_BCH_inter_DVBS2O(*this);
	m->deep_copy(*this);
	return m;
}

template <typename B>
void Encoder_BCH_inter_DVBS2O<B>
::_encode(const B *U_K, B *X_N, const int frame_id)
{
	// reverse bits for DVBS2 standard to aff3ct compliance
	for (auto f = 0; f < this->simd_inter_frame_level; f++)
		std::reverse_copy(U_K             + (f +0) * this->K,
		                  U_K             + (f +1) * this->K,
		                  U_K_rev.begin() + (f +0) * this->K);

	// generate the parity bits
	this->__encode(U_K_rev.data(), X_N);

	// copy sys bits
	for (auto f = 0; f < this->simd_inter_frame_level; f++)
		std::copy(U_K_rev.data() + (f +0) * this->K,
		          U_K_rev.data() + (f +1) * this->K,
		          X_N            + (f +0) * this->N + this->n_rdncy);

	// reverse bits for DVBS2 standard to aff3ct compliance
	for (auto f = 0; f < this->simd_inter_frame_level; f++)
		std::reverse(X_N + (f +0) * this->N,
		             X_N + (f +0) * this->N + this->K + this->n_rdncy);
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
#ifdef AFF3CT_MULTI_PREC
template class aff3ct::module::Encoder_BCH_inter_DVBS2O<B_8>;
template class aff3ct::module::Encoder_BCH_inter_DVBS2O<B_16>;
template class aff3ct::module::Encoder_BCH_inter_DVBS2O<B_32>;
template class aff3ct::module::Encoder_BCH_inter_DVBS2O<B_64>;
#else
template class aff3ct::module::Encoder_BCH_inter_DVBS2O<B>;
#endif
// ==================================================================================== explicit template instantiation

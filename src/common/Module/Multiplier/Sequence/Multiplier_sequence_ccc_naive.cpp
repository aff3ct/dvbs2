#include <cassert>
#include <iostream>
#include <vector>
#include <complex>
#include <cmath>

#include "Multiplier_sequence_ccc_naive.hpp"
using namespace aff3ct::module;

Multiplier_sequence_ccc_naive
::Multiplier_sequence_ccc_naive(const int N, const std::vector<float>& sequence, const int n_frames)
: Multiplier<float>(N, n_frames), cplx_sequence(N/2)
{	if (N != (int)sequence.size())
	{
		std::stringstream message;
		message << "'sequence.size()' has to be equal to 'N' ('sequence.size()' = " << sequence.size()
		        << ", 'N' = " << this->N << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}
	for (auto i=0 ; i< N/2; i++)
		this->cplx_sequence[i] = std::complex<float>(sequence[2*i],sequence[2*i+1]);
}

Multiplier_sequence_ccc_naive
::~Multiplier_sequence_ccc_naive()
{}

void Multiplier_sequence_ccc_naive
::_imultiply(const float *X_N,  float *Z_N, const int frame_id)
{
	const std::complex<float>* cX_N = reinterpret_cast<const std::complex<float>* >(X_N);
	std::complex<float>* cZ_N = reinterpret_cast<std::complex<float>* >(Z_N);
	for (auto i = 0 ; i < this->N/2 ; i++)
		cZ_N[i] = cX_N[i] * this->cplx_sequence[i];
}

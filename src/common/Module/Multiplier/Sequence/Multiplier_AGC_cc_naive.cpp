#include <cassert>
#include <iostream>
#include <vector>
#include <complex>
#include <cmath>

#include "Module/Multiplier/Sequence/Multiplier_AGC_cc_naive.hpp"
using namespace aff3ct::module;

template <typename R>
Multiplier_AGC_cc_naive<R>
::Multiplier_AGC_cc_naive(const int N, const R output_energy, const int n_frames)
: Multiplier<R>(N, n_frames), output_energy(output_energy)
{
}

template <typename R>
Multiplier_AGC_cc_naive<R>
::~Multiplier_AGC_cc_naive()
{}

template <typename R>
void Multiplier_AGC_cc_naive<R>
::_imultiply(const R *X_N,  R *Z_N, const int frame_id)
{
	R sum_abs_2 = 0.0;
	R sum_val_re = 0.0;
	R sum_val_im = 0.0;
	int N_cplx = this->N/2;
	for (int i = 0 ; i < N_cplx ; i++)
	{
		sum_abs_2  += X_N[2*i]*X_N[2*i] + X_N[2*i+1]*X_N[2*i+1];
		sum_val_re += X_N[2*i];
		sum_val_im += X_N[2*i+1];
	}

	R std_xn = sqrt(sum_abs_2 * (R)N_cplx - sum_val_re*sum_val_re - sum_val_im * sum_val_im) / (R)N_cplx;
	std_xn /= sqrt(output_energy);

	for (int i = 0 ; i < N_cplx ; i++)
	{
		Z_N[2*i    ] = X_N[2*i    ] / std_xn;
		Z_N[2*i + 1] = X_N[2*i + 1] / std_xn;
	}
}

template class aff3ct::module::Multiplier_AGC_cc_naive<float>;
template class aff3ct::module::Multiplier_AGC_cc_naive<double>;
#include <cassert>
#include <iostream>

#include "Synchronizer_coarse_freq_NO.hpp"

using namespace aff3ct::module;

template <typename R>
Synchronizer_coarse_freq_NO<R>
::Synchronizer_coarse_freq_NO(const int N, const int n_frames)
:Synchronizer_coarse_freq<R>(N, n_frames)
{
}

template <typename R>
Synchronizer_coarse_freq_NO<R>
::~Synchronizer_coarse_freq_NO()
{}

template <typename R>
void Synchronizer_coarse_freq_NO<R>
::_synchronize(const R *X_N1, R *Y_N2, const int frame_id)
{
	for (int i = 0 ; i<this->N_in ; i++)
		Y_N2[i] = X_N1[i];
}

template <typename R>
void Synchronizer_coarse_freq_NO<R>
::step(const std::complex<R>* x_elt, std::complex<R>* y_elt)
{
	*y_elt = *x_elt;
}


template <typename R>
void Synchronizer_coarse_freq_NO<R>
::update_phase(const std::complex<R> spl)
{
}

template <typename R>
void Synchronizer_coarse_freq_NO<R>
::set_PLL_coeffs (const int pll_sps, const R damping_factor, const R normalized_bandwidth)
{
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Synchronizer_coarse_freq_NO<float>;
template class aff3ct::module::Synchronizer_coarse_freq_NO<double>;
// ==================================================================================== explicit template instantiation

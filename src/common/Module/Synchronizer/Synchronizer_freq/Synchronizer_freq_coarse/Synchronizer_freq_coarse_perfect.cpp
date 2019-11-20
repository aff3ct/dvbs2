#include <cassert>
#include <iostream>

#include "Synchronizer_freq_coarse_perfect.hpp"

using namespace aff3ct::module;

template <typename R>
Synchronizer_freq_coarse_perfect<R>
::Synchronizer_freq_coarse_perfect(const int N)
:Synchronizer_freq_coarse<R>(N)
{
}

template <typename R>
Synchronizer_freq_coarse_perfect<R>
::~Synchronizer_freq_coarse_perfect()
{}

template <typename R>
void Synchronizer_freq_coarse_perfect<R>
::_synchronize(const R *X_N1, R *Y_N2, const int frame_id)
{
	for (int i = 0 ; i<this->N_in ; i++)
		Y_N2[i] = X_N1[i];
}

template <typename R>
void Synchronizer_freq_coarse_perfect<R>
::step(const std::complex<R>* x_elt, std::complex<R>* y_elt)
{
	*y_elt = *x_elt;
}


template <typename R>
void Synchronizer_freq_coarse_perfect<R>
::update_phase(const std::complex<R> spl)
{
}

template <typename R>
void Synchronizer_freq_coarse_perfect<R>
::set_PLL_coeffs (const int pll_sps, const R damping_factor, const R normalized_bandwidth)
{
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Synchronizer_freq_coarse_perfect<float>;
template class aff3ct::module::Synchronizer_freq_coarse_perfect<double>;
// ==================================================================================== explicit template instantiation

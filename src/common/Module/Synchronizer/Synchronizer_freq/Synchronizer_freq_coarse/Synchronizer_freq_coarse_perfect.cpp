#include <cassert>
#include <iostream>

#include "Synchronizer_freq_coarse_perfect.hpp"

using namespace aff3ct::module;

template <typename R>
Synchronizer_freq_coarse_perfect<R>
::Synchronizer_freq_coarse_perfect(const int N, const R frequency_offset, const int n_frames)
:Synchronizer_freq_coarse<R>(N, n_frames), mult(N, -frequency_offset, (R)1.0, 1)
{
	this->estimated_freq = frequency_offset;
}

template <typename R>
Synchronizer_freq_coarse_perfect<R>
::~Synchronizer_freq_coarse_perfect()
{}

template <typename R>
void Synchronizer_freq_coarse_perfect<R>
::_synchronize(const R *X_N1, R *Y_N2, const int frame_id)
{
	this->mult.imultiply(X_N1, Y_N2, frame_id);
}

template <typename R>
void Synchronizer_freq_coarse_perfect<R>
::step(const std::complex<R>* x_elt, std::complex<R>* y_elt)
{
	this->mult.step(x_elt, y_elt);
}

template <typename R>
void Synchronizer_freq_coarse_perfect<R>
::_reset()
{
	this->mult.reset();
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

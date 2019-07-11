#include <cassert>
#include <iostream>
#include <vector>
#include <complex>
#define _USE_MATH_DEFINES // enable M_PI definition on MSVC compiler
#include <cmath>

// _USE_MATH_DEFINES does not seem to work on MSVC...
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#include "Multiplier_sine_ccc_naive.hpp"

using namespace aff3ct::module;

template <typename R>
Multiplier_sine_ccc_naive<R>
::Multiplier_sine_ccc_naive(const int N, const R f, const R Fs, const int n_frames)
: Multiplier<R>(N, n_frames), n(0.0f), f(f), omega(2 * M_PI * f/Fs), Fs(Fs)
{}

template <typename R>
Multiplier_sine_ccc_naive<R>
::~Multiplier_sine_ccc_naive()
{}

template <typename R>
void Multiplier_sine_ccc_naive<R>
::set_f(R f)
{
	this-> f = f;
	this->omega = 2 * M_PI * f/this->Fs;
}

template <typename R>
R Multiplier_sine_ccc_naive<R>
::get_nu()
{
	return this->f/this->Fs;
}

template <typename R>
void Multiplier_sine_ccc_naive<R>
::reset_time()
{
	this->n = 0.0f;
}

template <typename R>
inline void Multiplier_sine_ccc_naive<R>
::step(const std::complex<R>* x_elt, std::complex<R>* y_elt)
{
	R phase(this->omega * this->n);
	*y_elt = *x_elt * std::complex<R>(std::cos(phase), std::sin(phase));
	this->n += 1.0f;
}

template <typename R>
void Multiplier_sine_ccc_naive<R>
::_imultiply(const R *X_N,  R *Z_N, const int frame_id)
{
	const std::complex<R>* cX_N = reinterpret_cast<const std::complex<R>* >(X_N);
	std::complex<R>* cZ_N = reinterpret_cast<std::complex<R>* >(Z_N);
	for (auto i = 0 ; i < this->N/2 ; i++)
		this->step(&cX_N[i], &cZ_N[i]);
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Multiplier_sine_ccc_naive<float>;
template class aff3ct::module::Multiplier_sine_ccc_naive<double>;
// ==================================================================================== explicit template instantiation

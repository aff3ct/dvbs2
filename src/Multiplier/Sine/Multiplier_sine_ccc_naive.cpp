#include <cassert>
#include <iostream>
#include <vector>
#include <complex>
#define _USE_MATH_DEFINES // enable M_PI definition on MSVC compiler
#include <cmath>

#include "Multiplier_sine_ccc_naive.hpp"

using namespace aff3ct::module;

Multiplier_sine_ccc_naive
::Multiplier_sine_ccc_naive(const int N, const float f, const float Fs, const int n_frames)
: Multiplier<float>(N, n_frames), n(0.0f), f(f), omega(2 * M_PI * f/Fs), Fs(Fs)
{}

Multiplier_sine_ccc_naive
::~Multiplier_sine_ccc_naive()
{}

void Multiplier_sine_ccc_naive
::set_f(float f)
{
	this-> f = f;
	this->omega = 2 * M_PI * f/this->Fs;
}

void Multiplier_sine_ccc_naive
::reset_time()
{
	this->n = 0.0f;
}

inline void Multiplier_sine_ccc_naive
::step(const std::complex<float>* x_elt, std::complex<float>* y_elt)
{
	float phase(this->omega * this->n);
	*y_elt = *x_elt * std::complex<float>(std::cos(phase), std::sin(phase));
	this->n += 1.0f;
}

void Multiplier_sine_ccc_naive
::_imultiply(const float *X_N,  float *Z_N, const int frame_id)
{
	const std::complex<float>* cX_N = reinterpret_cast<const std::complex<float>* >(X_N);
	std::complex<float>* cZ_N = reinterpret_cast<std::complex<float>* >(Z_N);
	for (auto i = 0 ; i < this->N/2 ; i++)
		this->step(&cX_N[i], &cZ_N[i]);
}
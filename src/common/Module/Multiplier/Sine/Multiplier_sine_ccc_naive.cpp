#define _USE_MATH_DEFINES // enable M_PI definition on MSVC compiler
#include <cmath>

// _USE_MATH_DEFINES does not seem to work on MSVC...
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#include "Module/Multiplier/Sine/Multiplier_sine_ccc_naive.hpp"

using namespace aff3ct::module;

template <typename R>
Multiplier_sine_ccc_naive<R>
::Multiplier_sine_ccc_naive(const int N, const R f, const R Fs, const int n_frames)
: Multiplier<R>(N, n_frames), n(0), f(0), nu(0), omega(0), Fs(Fs)
{
	R new_nu = std::floor(f/Fs * (R)1e6) /(R)1e6;
	this->nu = new_nu;
	this->f  = new_nu * this->Fs;
	this->omega = 2 * M_PI * new_nu;
}

template <typename R>
Multiplier_sine_ccc_naive<R>
::~Multiplier_sine_ccc_naive()
{}

template <typename R>
void Multiplier_sine_ccc_naive<R>
::set_f(R f)
{
	R new_nu = std::floor(f/Fs * (R)1e6) /(R)1e6;
	this->nu = new_nu;
	this->f  = new_nu * this->Fs;
	this->omega = 2 * M_PI * new_nu;
}

template <typename R>
void Multiplier_sine_ccc_naive<R>
::set_omega(R omega)
{
	R new_nu = std::floor(omega / (2 * M_PI) * (R)1e6) /(R)1e6;
	this->nu = new_nu;
	this->f  = new_nu * this->Fs;
	this->omega = 2 * M_PI * new_nu;
}

template <typename R>
void Multiplier_sine_ccc_naive<R>
::set_nu(R nu)
{
	R new_nu = std::floor(nu * (R)1e6) /(R)1e6;
	this->nu = new_nu;
	this->f  = new_nu * this->Fs;
	this->omega = 2 * M_PI * new_nu;
}

template <typename R>
R Multiplier_sine_ccc_naive<R>
::get_nu()
{
	return this->nu;
}

template <typename R>
void Multiplier_sine_ccc_naive<R>
::reset()
{
	this->n = (R)0.0;
}

template <typename R>
inline void Multiplier_sine_ccc_naive<R>
::step(const std::complex<R>* x_elt, std::complex<R>* y_elt)
{
	R phase(this->omega * this->n);
	*y_elt = *x_elt * std::complex<R>(std::cos(phase), std::sin(phase));

	this->n = (this->n >= 999999) ? 0 : this->n + (R)1.0;
	// std::cerr << "alive" << std::endl;
}

template <typename R>
void Multiplier_sine_ccc_naive<R>
::_imultiply(const R *X_N,  R *Z_N, const int frame_id)
{
	const std::complex<R>* cX_N = reinterpret_cast<const std::complex<R>* >(X_N);
	std::complex<R>*       cZ_N = reinterpret_cast<      std::complex<R>* >(Z_N);
	for (auto i = 0 ; i < this->N/2 ; i++)
		this->step(&cX_N[i], &cZ_N[i]);
	//std::cerr << (int)this->n << "|" << phase << " | " << std::cos(phase)<< "|" << std::sin(phase)<<std::endl;
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Multiplier_sine_ccc_naive<float>;
template class aff3ct::module::Multiplier_sine_ccc_naive<double>;
// ==================================================================================== explicit template instantiation

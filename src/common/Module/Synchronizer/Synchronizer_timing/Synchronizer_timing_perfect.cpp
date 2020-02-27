#include <cassert>
#include <iostream>

#include "aff3ct.hpp"
#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_timing_perfect.hpp"

// _USE_MATH_DEFINES does not seem to work on MSVC...
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

using namespace aff3ct::module;

template <typename B, typename R>
Synchronizer_timing_perfect<B,R>
::Synchronizer_timing_perfect(const int N, const int osf, const R channel_delay, const int n_frames)
: Synchronizer_timing<B,R>(N, osf, n_frames),
farrow_flt   (N, (R)0),
NCO_counter_0((R)0),
NCO_counter  ((R)0)
{
	R frac_delay = channel_delay - std::floor(channel_delay);
	R int_delay  = channel_delay - frac_delay + (R)3;

	NCO_counter_0 = osf - (R)((int)int_delay % osf);
	NCO_counter   = NCO_counter_0;
	this->is_strobe = ((int)this->NCO_counter % this->osf == 0) ? 1:0;
	this->mu = (R)1-frac_delay;
	this->farrow_flt.set_mu(this->mu);
}

template <typename B, typename R>
Synchronizer_timing_perfect<B,R>
::~Synchronizer_timing_perfect()
{
}

template <typename B, typename R>
void Synchronizer_timing_perfect<B,R>
::_synchronize(const R *X_N1, R *Y_N1, B *B_N1, const int frame_id)
{
	auto cX_N1 = reinterpret_cast<const std::complex<R>* >(X_N1);
	auto cY_N1 = reinterpret_cast<      std::complex<R>* >(Y_N1);

	for (auto i = 0; i < this->N_in/2 ; i++)
		this->step(&cX_N1[i], &cY_N1[i], &B_N1[2*i]);
}

template <typename B, typename R>
void Synchronizer_timing_perfect<B,R>
::_reset()
{
	this->farrow_flt.reset();
	this->farrow_flt.set_mu(this->mu);

	this->NCO_counter = this->NCO_counter_0;
	this->is_strobe = ((int)this->NCO_counter % this->osf == 0) ? 1:0;
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Synchronizer_timing_perfect<int, float>;
template class aff3ct::module::Synchronizer_timing_perfect<int, double>;
// ==================================================================================== explicit template instantiation

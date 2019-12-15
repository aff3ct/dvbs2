#include <cassert>
#include <iostream>

#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_timing_perfect.hpp"

// _USE_MATH_DEFINES does not seem to work on MSVC...
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

using namespace aff3ct::module;

template <typename R>
Synchronizer_timing_perfect<R>
::Synchronizer_timing_perfect(const int N, const int osf, const R channel_delay, const int n_frames)
: Synchronizer_timing<R>(N, osf, n_frames),
farrow_flt   (N, (R)0),
NCO_counter_0((R)0),
NCO_counter  ((R)0)
{
	//01230123
	//3012301230123
	R frac_delay = channel_delay - std::floor(channel_delay);
	R int_delay  = channel_delay - frac_delay + (R)1;

	NCO_counter_0 = osf - (R)((int)int_delay % osf);
	NCO_counter   = NCO_counter_0;
	this->mu = (R)1-frac_delay;
	this->farrow_flt.set_mu(this->mu);
}

template <typename R>
Synchronizer_timing_perfect<R>
::~Synchronizer_timing_perfect()
{}

template <typename R>
void Synchronizer_timing_perfect<R>
::_synchronize(const R *X_N1, R *Y_N2, const int frame_id)
{
	auto cX_N1 = reinterpret_cast<const std::complex<R>* >(X_N1);
	auto cY_N2 = reinterpret_cast<      std::complex<R>* >(Y_N2);

	for (auto i = 0; i < this->N_in/2 ; i++)
		this->step(&cX_N1[i]);

	for (auto i = 0; i < this->N_out/2; i++)
		this->pull(&cY_N2[i]);
}

template <typename R>
void Synchronizer_timing_perfect<R>
::_sync_push (const R *X_N1, const int frame_id)
{
	auto cX_N1 = reinterpret_cast<const std::complex<R>* >(X_N1);

	for (auto i = 0; i < this->N_in/2 ; i++)
		this->step(&cX_N1[i]);
}
template <typename R>
void Synchronizer_timing_perfect<R>
::_sync_pull (R *Y_N2, const int frame_id)
{
	auto cY_N2 = reinterpret_cast<      std::complex<R>* >(Y_N2);

	for (auto i = 0; i < this->N_out/2; i++)
		this->pull(&cY_N2[i]);
}

template <typename R>
void Synchronizer_timing_perfect<R>
::step(const std::complex<R> *X_N1)
{
	this->is_strobe = ((int)this->NCO_counter % this->osf == 0) ? 1:0; // Check if a strobe

	std::complex<R> farrow_output(0,0);
	farrow_flt.step( X_N1, &farrow_output);
	if (this->is_strobe)
	{
		this->push(farrow_output);
		this->last_symbol = farrow_output;
	}

	this->NCO_counter += 1.0f;
	this->NCO_counter = (R)((int)this->NCO_counter % this->osf);

}

template <typename R>
void Synchronizer_timing_perfect<R>
::_reset()
{
	this->farrow_flt.reset();
	this->farrow_flt.set_mu(this->mu);

	this->NCO_counter = this->NCO_counter_0;
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Synchronizer_timing_perfect<float>;
template class aff3ct::module::Synchronizer_timing_perfect<double>;
// ==================================================================================== explicit template instantiation

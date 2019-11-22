#include <cassert>
#include <iostream>

#include "Module/Synchronizer/Synchronizer_frame/Synchronizer_frame_perfect.hpp"

// _USE_MATH_DEFINES does not seem to work on MSVC...
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

using namespace aff3ct::module;

template <typename R>
Synchronizer_frame_perfect<R>
::Synchronizer_frame_perfect(const int N, const int frame_delay)
: Synchronizer_frame<R>(N), output_delay(N, N/2-frame_delay, N/2-frame_delay)
{
	this->delay = frame_delay;
}

template <typename R>
Synchronizer_frame_perfect<R>
::~Synchronizer_frame_perfect()
{}

template <typename R>
void Synchronizer_frame_perfect<R>
::_synchronize(const R *X_N1, R *Y_N2, const int frame_id)
{
	this->output_delay.filter(X_N1,Y_N2);
}

template <typename R>
void Synchronizer_frame_perfect<R>
::reset()
{
	this->output_delay.reset();
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Synchronizer_frame_perfect<float>;
template class aff3ct::module::Synchronizer_frame_perfect<double>;
// ==================================================================================== explicit template instantiation

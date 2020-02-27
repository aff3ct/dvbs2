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
::Synchronizer_frame_perfect(const int N, const int frame_delay, const int n_frames)
: Synchronizer_frame<R>(N, n_frames), frame_delay(frame_delay), output_delay(N, N / 2 - frame_delay, N / 2 - frame_delay)
{
}

template <typename R>
Synchronizer_frame_perfect<R>
::~Synchronizer_frame_perfect()
{}

template <typename R>
void Synchronizer_frame_perfect<R>
::_synchronize(const R *X_N1, int* delay, R *Y_N2, const int frame_id)
{
	*delay = this->frame_delay;
	this->delay = *delay;
	this->output_delay.filter(X_N1,Y_N2);
}

template <typename R>
void Synchronizer_frame_perfect<R>
::reset()
{
	this->output_delay.reset();
}

template <typename R>
R Synchronizer_frame_perfect<R>
::_get_metric() const
{
	return (R)0;
};

template <typename R>
bool Synchronizer_frame_perfect<R>
::_get_packet_flag() const
{
	return true;
};

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Synchronizer_frame_perfect<float>;
template class aff3ct::module::Synchronizer_frame_perfect<double>;
// ==================================================================================== explicit template instantiation

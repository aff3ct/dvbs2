#include <cassert>
#include <iostream>

#include "Synchronizer_freq_fine_perfect.hpp"

// _USE_MATH_DEFINES does not seem to work on MSVC...
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

using namespace aff3ct::module;

template <typename R>
Synchronizer_freq_fine_perfect<R>
::Synchronizer_freq_fine_perfect(const int N, const R frequency_offset, const R phase_offset, const int n_frames)
:Synchronizer_freq_fine<R>(N, n_frames)
{
	this->estimated_freq  = frequency_offset;
	this->estimated_phase = phase_offset;
}

template <typename R>
Synchronizer_freq_fine_perfect<R>
::~Synchronizer_freq_fine_perfect()
{}

template <typename R>
Synchronizer_freq_fine_perfect<R>* Synchronizer_freq_fine_perfect<R>
::clone() const
{
	auto m = new Synchronizer_freq_fine_perfect(*this);
	m->deep_copy(*this);
	return m;
}

template <typename R>
void Synchronizer_freq_fine_perfect<R>
::_synchronize(const R *X_N1, R *Y_N2, const int frame_id)
{
	for (int n = 0 ; n < this->N/2 ; n++)
	{
		R theta = 2 * M_PI *(this->estimated_freq * (R)n + this->estimated_phase);

		Y_N2[2*n    ] = X_N1[2*n    ] * std::cos(theta)
		              + X_N1[2*n + 1] * std::sin(theta);

		Y_N2[2*n + 1] = X_N1[2*n + 1] * std::cos(theta)
		              - X_N1[2*n    ] * std::sin(theta);
	}
}

template <typename R>
void Synchronizer_freq_fine_perfect<R>
::_reset()
{
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Synchronizer_freq_fine_perfect<float>;
template class aff3ct::module::Synchronizer_freq_fine_perfect<double>;
// ==================================================================================== explicit template instantiation

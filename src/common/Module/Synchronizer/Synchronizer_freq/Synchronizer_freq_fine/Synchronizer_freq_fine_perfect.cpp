#include <cassert>
#include <iostream>

#include "Synchronizer_freq_fine_perfect.hpp"

using namespace aff3ct::module;

template <typename R>
Synchronizer_freq_fine_perfect<R>
::Synchronizer_freq_fine_perfect(const int N, const R frequency_offset, const R phase_offset)
:Synchronizer_freq<R>(N)
{
	this->estimated_freq  = frequency_offset;
	this->estimated_phase = phase_offset;
}

template <typename R>
Synchronizer_freq_fine_perfect<R>
::~Synchronizer_freq_fine_perfect()
{}

template <typename R>
void Synchronizer_freq_fine_perfect<R>
::_synchronize(const R *X_N1, R *Y_N2, const int frame_id)
{
	for (int n = 0 ; n < this->N_in/2 ; n++)
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
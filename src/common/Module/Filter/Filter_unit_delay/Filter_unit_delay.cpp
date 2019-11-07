#include <cassert>
#include <iostream>
#include <vector>
#include <complex>

#include "Filter_unit_delay.hpp"
using namespace aff3ct::module;

template <typename R>
Filter_unit_delay<R>
::Filter_unit_delay(const int N, const int n_frames)
: Filter<R>(N, N, n_frames), mem(N, R(0))
{
}

template <typename R>
Filter_unit_delay<R>
::~Filter_unit_delay()
{}

template <typename R>
void Filter_unit_delay<R>
::_filter(const R *X_N1, R *Y_N2, const int frame_id)
{
	for(auto i = 0; i < this->N; i++)
	{
		Y_N2[i]      = this->mem[i];
		this->mem[i] = X_N1[i];
	}
}


template <typename R>
void Filter_unit_delay<R>
::reset()
{
	for (auto i = 0; i < this->N; i++)
		this->mem[i] = (R)0;
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Filter_unit_delay<int>;
template class aff3ct::module::Filter_unit_delay<float>;
template class aff3ct::module::Filter_unit_delay<double>;
// ==================================================================================== explicit template instantiation

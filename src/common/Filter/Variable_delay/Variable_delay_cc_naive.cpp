#include <cassert>
#include <iostream>
#include <vector>
#include <complex>

#include "Variable_delay_cc_naive.hpp"
using namespace aff3ct::module;

template <typename R>
Variable_delay_cc_naive<R>
::Variable_delay_cc_naive(const int N, const int delay, const int max_delay)
: Filter<R>(N,N), d(d), buff(2*(max_delay+1), std::complex<R>(R(0))), head(0), size(max_delay+1)
{
	assert(size > 0);
}

template <typename R>
Variable_delay_cc_naive<R>
::~Variable_delay_cc_naive()
{}

template <typename R>
void Variable_delay_cc_naive<R>
::_filter(const R *X_N1, R *Y_N2, const int frame_id)
{
	auto cX_N1 = reinterpret_cast<const std::complex<R>* >(X_N1);
	auto cY_N2 = reinterpret_cast<std::complex<R>* >(Y_N2);
	for(auto i = 0; i<this->N/2; i++)
		step(cX_N1 + i, cY_N2 + i);
}


template <typename R>
void Variable_delay_cc_naive<R>
::reset()
{
	for (size_t i = 0; i<this->buff.size(); i++)
		this->buff[i] = std::complex<R>(R(0));

	this->head = 0;
}

template <typename R>
void Variable_delay_cc_naive<R>
::set_delay(const int delay)
{
	this->delay = delay < this->size ? delay : this->size - 1;
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Variable_delay_cc_naive<float>;
template class aff3ct::module::Variable_delay_cc_naive<double>;
// ==================================================================================== explicit template instantiation

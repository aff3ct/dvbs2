#include <cassert>
#include <iostream>
#include <vector>
#include <complex>

#include "Filter_FIR_ccr_naive.hpp"
using namespace aff3ct::module;

template <typename R>
Filter_FIR_ccr_naive<R>
::Filter_FIR_ccr_naive(const int N, const std::vector<R> b)
: Filter<R>(N,N), b(b.size(), R(0)), buff(2*b.size(), std::complex<R>(R(0))), head(0), size((int)b.size())
{
	assert(size > 0);

	for (size_t i = 0; i < b.size(); i++)
		this->b[i] = b[b.size() - 1 - i];
}

template <typename R>
Filter_FIR_ccr_naive<R>
::~Filter_FIR_ccr_naive()
{}

template <typename R>
void Filter_FIR_ccr_naive<R>
::_filter(const R *X_N1, R *Y_N2, const int frame_id)
{
	auto cX_N1 = reinterpret_cast<const std::complex<R>* >(X_N1);
	auto cY_N2 = reinterpret_cast<std::complex<R>* >(Y_N2);
	for(auto i = 0; i<this->N/2; i++)
		step(cX_N1 + i, cY_N2 + i);
}


template <typename R>
void Filter_FIR_ccr_naive<R>
::reset()
{
	for (size_t i = 0; i<this->buff.size(); i++)
		this->buff[i] = std::complex<R>(R(0));

	this->head = 0;
}

template <typename R>
std::vector<R> Filter_FIR_ccr_naive<R>
::get_filter_coefs()
{
	std::vector<R> flipped_b(this->b.size(),(R)0);
	for (size_t i=0; i<this->b.size();i++)
		flipped_b[i] = this->b[this->b.size()-1-i];

	return flipped_b;
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Filter_FIR_ccr_naive<float>;
template class aff3ct::module::Filter_FIR_ccr_naive<double>;
// ==================================================================================== explicit template instantiation

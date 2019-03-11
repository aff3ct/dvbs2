#ifndef FILTER_FIR_CCR_NAIVE_HXX
#define FILTER_FIR_CCR_NAIVE_HXX

#include <vector>
#include <complex>

#include "Filter_FIR_ccr_naive.hpp"

namespace aff3ct
{
namespace module
{
template <typename R>
Filter_FIR_ccr_naive<R>
::Filter_FIR_ccr_naive(const int N, const std::vector<R> b)
: Filter<R>(N,N), b(b.size(), R(0)), buff(2*b.size(), std::complex<R>(R(0))), head(0), size(b.size())
{
	assert(size > 0);

	for (auto i = 0; i < b.size(); i++)
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
	for (auto i = 0; i<this->buff.size(); i++)
		this->buff[i] = std::complex<R>(R(0));

	this->head = 0;
}


template <typename R>
void Filter_FIR_ccr_naive<R>
::step(const std::complex<R>* x_elt, std::complex<R>* y_elt)
{
	this->buff[this->head] = *x_elt;
	this->buff[this->head + this->size] = *x_elt;
	
	std::complex<R> ps = this->buff[this->head+1] * this->b[0];
	for (auto i = 1; i < this->size ; i++)
		ps += this->buff[this->head+1+i] * this->b[i];

	*y_elt = ps;
	
	this->head++;
	this->head %= this->size;
}
template <typename R>
std::vector<R> Filter_FIR_ccr_naive<R>
::get_filter_coefs()
{
	std::vector<R> flipped_b(this->b.size(),(R)0);
	for (auto i=0; i<this->b.size();i++)
		flipped_b[i] = this->b[this->b.size()-1-i];

	return flipped_b;
}

}
}
#endif //FILTER_FIR_CCR_NAIVE_HXX
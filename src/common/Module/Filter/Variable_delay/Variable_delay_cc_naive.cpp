#include <cassert>
#include <iostream>
#include <vector>
#include <complex>
#include <algorithm>

#include "Module/Filter/Variable_delay/Variable_delay_cc_naive.hpp"

using namespace aff3ct::module;

template <typename R>
Variable_delay_cc_naive<R>
::Variable_delay_cc_naive(const int N, const int delay, const int max_delay, const int n_frames)
: Filter<R>(N, N, n_frames),
  delay(delay),
  buff(2*(max_delay+1), std::complex<R>(R(0))),
  buff2(4*(max_delay+1), R(0)),
  head(0),
  head2(0),
  size(max_delay+1),
  first_time(true)
{
	assert(size > 0);
}

template <typename R>
Variable_delay_cc_naive<R>
::~Variable_delay_cc_naive()
{
}

template <typename R>
void Variable_delay_cc_naive<R>
::_filter_old(const R *X_N1, R *Y_N2, const int frame_id)
{
	auto cX_N1 = reinterpret_cast<const std::complex<R>* >(X_N1);
	auto cY_N2 = reinterpret_cast<std::complex<R>* >(Y_N2);
	for(auto i = 0; i<this->N/2; i++)
		step(cX_N1 + i, cY_N2 + i);

	// for (auto i = 0; i < this->N; i++)
	// {
	// 	this->buff2[this->head2] = X_N1[i];
	// 	this->buff2[this->head2 + 2*this->size] = X_N1[i];

	// 	Y_N2[i] = this->buff2[this->head2 + 2*this->size-2*this->delay];

	// 	this->head2++;
	// 	this->head2 %= 2*this->size;
	// }
}

template <typename R>
void Variable_delay_cc_naive<R>
::_filter(const R *X_N1, R *Y_N2, const int frame_id)
{
	assert(this->N >= 2*(this->size -1));

	auto start_Y_N2 = (2*this->delay > this->head2) ? 2*this->delay - this->head2 : 0;
	auto start_buff = (2*this->delay < this->head2) ? this->head2 - 2*this->delay : 0;
	auto end_buff   = start_buff + 2*this->delay;
	     end_buff   = end_buff > (int)this->buff2.size() ? this->buff2.size() : end_buff;
	     end_buff   = (end_buff - start_buff > this->N - start_Y_N2) ?
	                  end_buff - ((end_buff - start_buff) - (this->N - start_Y_N2)) : end_buff;

	if (start_Y_N2 && !this->first_time)
		std::copy(Y_N2 + this->N - start_Y_N2, Y_N2 + this->N, Y_N2);
	else
		std::fill(Y_N2, Y_N2 + start_Y_N2, (R)0);

	std::copy(this->buff2.begin() + start_buff, this->buff2.begin() + end_buff, Y_N2 + start_Y_N2);
	std::copy(X_N1, X_N1 + this->N - 2*this->delay, Y_N2 + 2*this->delay);
	std::copy(X_N1 + this->N - 2*this->delay, X_N1 + this->N, this->buff2.begin());

	this->first_time = false;
	this->head2 = 2*this->delay;
}

template <typename R>
void Variable_delay_cc_naive<R>
::reset()
{
	std::fill(this->buff .begin(), this->buff .end(), std::complex<R>(R(0),R(0)));
	std::fill(this->buff2.begin(), this->buff2.end(), R(0));
	this->head = 0;
	this->head2 = 0;
	this->first_time = true;
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

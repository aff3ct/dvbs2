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
  size(max_delay+1)
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
::_filter(const R *X_N1, R *Y_N2, const int frame_id)
{
	auto cX_N1 = reinterpret_cast<const std::complex<R>* >(X_N1);
	auto cY_N2 = reinterpret_cast<std::complex<R>* >(Y_N2);
	for(auto i = 0; i<this->N/2; i++)
		step(cX_N1 + i, cY_N2 + i);
}

// template <typename R>
// void Variable_delay_cc_naive<R>
// ::_filter(const R *X_N1, R *Y_N2, const int frame_id)
// {
// 	// std::copy(X_N1, X_N1 + this->N, this->buff.begin() + 2*this->delay);

// 	// std::copy(this->buff.begin(), this->buff.begin() + this->N, Y_N2);

// 	// std::copy(this->buff.begin() + 2*this->delay + this->buff.begin(),
// 	//           this->buff.end(),
// 	//           this->buff.begin());

// 	auto begin_Y_N2 = (2*this->delay > this->N) ? this->N : 2*this->delay;

// 	auto end_X_N1 = (this->N - 2*this->delay > 0) ? this->N - 2*this->delay : 0;

// 	std::cout << "2*this->delay = " << (2*this->delay) << std::endl;
// 	std::cout << "this->N = " << this->N << std::endl;
// 	std::cout << "begin_Y_N2 = " << begin_Y_N2 << std::endl;
// 	std::cout << "end_X_N1 = " << end_X_N1 << std::endl;
// 	std::cout << "this->head = " << this->head << std::endl;

// 	if (end_X_N1)
// 	{
// 		std::copy(X_N1,
// 	    	      X_N1 + end_X_N1,
// 	        	  Y_N2 + begin_Y_N2);
// 	}

// 	if (this->head)
// 	{
// 		std::copy(this->buff2.begin(),
// 		          this->buff2.begin() + begin_Y_N2,
// 		          Y_N2);

// 		std::copy(this->buff2.begin() + begin_Y_N2,
// 		          this->buff2.end(),
// 		          this->buff2.begin());

// 		this->head -= begin_Y_N2;
// 	}

// 	std::cout << "this->head = " << this->head << std::endl;

// 	std::copy(X_N1 + end_X_N1,
// 	          X_N1 + this->N,
// 	          this->buff2.begin() + this->head);

// 	this->head += this->N - end_X_N1;

// 	std::cout << "this->head = " << this->head << std::endl << std::endl;
// }

template <typename R>
void Variable_delay_cc_naive<R>
::reset()
{
	std::fill(this->buff .begin(), this->buff .end(), std::complex<R>(R(0),R(0)));
	std::fill(this->buff2.begin(), this->buff2.end(), R(0));
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

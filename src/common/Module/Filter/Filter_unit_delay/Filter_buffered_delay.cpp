#include <cassert>
#include <iostream>
#include <vector>
#include <complex>

#include "Filter_buffered_delay.hpp"
using namespace aff3ct::module;

template <typename R>
Filter_buffered_delay<R>
::Filter_buffered_delay(const int N, const int max_delay, const int delay, const int n_frames)
: Filter<R>(N, N, n_frames), mem(max_delay, std::vector<R>(N, R(0))), mem_heads(max_delay), delay(delay)
{
	for (size_t i = 0; i<this->mem.size(); i++)
		this->mem_heads[i] = this->mem[i].data();
}

template <typename R>
Filter_buffered_delay<R>
::~Filter_buffered_delay()
{}

template <typename R>
void Filter_buffered_delay<R>
::_filter(const R *X_N1, R *Y_N2, const int frame_id)
{
	if (this->delay > 0)
	{
		std::copy(this->mem_heads[0], this->mem_heads[0] + this->N, Y_N2);
		std::rotate(this->mem_heads.begin(), this->mem_heads.begin() + 1, this->mem_heads.end());
		std::copy(X_N1, X_N1 + this->N, this->mem_heads[this->delay-1]);
	}
	else
	{
		std::copy(X_N1, X_N1 + this->N, Y_N2);
	}

}

template <typename R>
void Filter_buffered_delay<R>
::reset()
{
	for (auto &m : mem)
		for (auto i = 0; i < this->N; i++)
			m[i] = (R)0;
}

template <typename R>
void Filter_buffered_delay<R>
::print_buffer()
{
	for (size_t j = 0; j< mem.size(); j++)
	{
		std::cout << "mem[" << j << "] " << +mem[j].data() << " | ";
		for (size_t i = 0; i < (size_t)this->N; i++)
		{
			std::cout << mem[j][i] << " ";
		}
		std::cout << std::endl;
	}

}
// ==================================================================================== explicit template instantiation
template class aff3ct::module::Filter_buffered_delay<int>;
template class aff3ct::module::Filter_buffered_delay<float>;
template class aff3ct::module::Filter_buffered_delay<double>;
// ==================================================================================== explicit template instantiation

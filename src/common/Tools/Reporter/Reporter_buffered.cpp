#include <utility>
#include <iomanip>
#include <sstream>
#include <cassert>
#include <ios>

#include "Reporter_buffered.hpp"

using namespace aff3ct;
using namespace aff3ct::tools;

template <typename R>
Reporter_buffered<R>
::Reporter_buffered(const int nb_cols, const int max_size)
: Reporter(),
head(nb_cols,0),
tail(nb_cols,0),
buffer(nb_cols, std::vector<R>(max_size + 1, (R)0)),
column_keys()
{
}

template <typename R>
void Reporter_buffered<R>
::push(const R *elt, int col)
{
	if (this->col_size(col) >= this->buffer[col].size())
		return;

	buffer[col][this->head[col]++] = *elt;
	this->head[col] %= buffer[col].size();
}

template <typename R>
void Reporter_buffered<R>
::push(const R *elt, std::string key)
{
	push(elt, this->column_keys[key]);
}

template <typename R>
void Reporter_buffered<R>
::pull(R *elt, int N, int col)
{
	int s  = N < this->col_size(col) ? N : this->col_size(col);
	int s1 = s < this->buffer[col].size() - this->tail[col] ? s : this->buffer[col].size() - this->tail[col];
	int s2 = s - s1;

	std::copy(&this->buffer[col][this->tail[col]], &this->buffer[col][this->tail[col]+s1], &elt[0 ]);
	std::copy(&this->buffer[col][0              ], &this->buffer[col][s2                ], &elt[s1]);
	this->tail[col] = (this->tail[col] + s) % this->buffer[col].size();
}

template <typename R>
void Reporter_buffered<R>
::pull(R *elt, int N, std::string key)
{
	pull(elt, N, this->column_keys[key]);
}

template <typename R>
void Reporter_buffered<R>
::print_buffer()
{
	for(int c=0; c<this->buffer.size(); c++)
	{
		std::cout << "[";
		for (int i = 0; i<this->buffer[c].size()-1; i++)
			std::cout << std::setprecision(4) << std::fixed << this->buffer[c][i] << ", ";
		std::cout << std::setprecision(4) << std::fixed << this->buffer[c][this->buffer[c].size()-1] << "]";

			std::cout << " H = " << this->head[c] << ", T = " << this->tail[c] << ", S = " << this->col_size(c)<< std::endl;

	}
	std::cout << std::endl;
}

template <typename R>
int Reporter_buffered<R>
::full_row_nbr()
{
	int min_size = this->col_size(0);

	for(int c=1; c<this->buffer.size(); c++)
		min_size = min_size > this->col_size(c) ? this->col_size(c) : min_size;

	return min_size;
}

template <typename R>
void Reporter_buffered<R>
::probe(std::string id)
{
	this->_probe(id);
}

template <typename R>
void Reporter_buffered<R>::
_probe(std::string id)
{
	throw tools::unimplemented_error(__FILE__, __LINE__, __func__);
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
#ifdef AFF3CT_MULTI_PREC
template class aff3ct::tools::Reporter_buffered<R_32>;
template class aff3ct::tools::Reporter_buffered<R_64>;
#else
template class aff3ct::tools::Reporter_buffered<R>;
#endif
// ==================================================================================== explicit template instantiation

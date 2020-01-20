#include "Module/Filter/Variable_delay/Variable_delay_cc_naive.hpp"

namespace aff3ct
{
namespace module
{

template <typename R>
void Variable_delay_cc_naive<R>
::step(const std::complex<R>* x_elt, std::complex<R>* y_elt)
{
	this->buff[this->head] = *x_elt;
	this->buff[this->head + this->size] = *x_elt;

	*y_elt = this->buff[this->head+this->size-this->delay];

	this->head++;
	this->head %= this->size;
}

}
}

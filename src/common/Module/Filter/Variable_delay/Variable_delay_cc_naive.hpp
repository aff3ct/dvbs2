#ifndef VARIABLE_DELAY_CC_NAIVE_HPP
#define VARIABLE_DELAY_CC_NAIVE_HPP

#include <vector>
#include <complex>

#include "Module/Filter/Filter.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Variable_delay_cc_naive : public Filter<R>
{
private:
	int delay;
	std::vector<std::complex<R> > buff;
	std::vector<R> buff2;
	int head;
	int size;

public:
	Variable_delay_cc_naive (const int N, const int delay, const int max_delay, const int n_frames = 1);
	virtual ~Variable_delay_cc_naive();
	inline void step(const std::complex<R>* x_elt, std::complex<R>* y_elt);
	void reset();
	void set_delay(const int delay);

protected:
	void _filter(const R *X_N1,  R *Y_N2, const int frame_id);

};

// Adrien: I put this function here because you wanted to be inlined
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

#endif //VARIABLE_DELAY_CC_NAIVE_HPP

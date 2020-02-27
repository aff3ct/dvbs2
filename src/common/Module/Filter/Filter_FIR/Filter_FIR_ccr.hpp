#ifndef FILTER_FIR_CCR_HPP
#define FILTER_FIR_CCR_HPP

#include <vector>
#include <complex>

#include "Module/Filter/Filter.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Filter_FIR_ccr : public Filter<R>
{
protected:
	std::vector<std::complex<R> > buff;
	int head;
	int size;
	int M;
	int P;

public:
	Filter_FIR_ccr (const int N, const std::vector<R> b, const int n_frames = 1);
	virtual ~Filter_FIR_ccr();
	inline void step(const std::complex<R>* x_elt, std::complex<R>* y_elt);
	void reset();
	std::vector<R> get_filter_coefs();

protected:
	std::vector<R> b;
	void _filter(const R *X_N1,  R *Y_N2, const int frame_id);
	void _filter_old(const R *X_N1,  R *Y_N2, const int frame_id);

};

// Adrien: I put this function here because you wanted to be inlined
template <typename R>
void Filter_FIR_ccr<R>
::step(const std::complex<R>* x_elt, std::complex<R>* y_elt)
{
	this->buff[this->head] = *x_elt;
	this->buff[this->head + this->size] = *x_elt;

	*y_elt = this->buff[this->head+1] * this->b[0];
	for (auto i = 1; i < this->size ; i++)
		*y_elt += this->buff[this->head + 1 + i] * this->b[i];

	this->head++;
	this->head %= this->size;
}

}
}

#endif //FILTER_FIR_CCR_HPP

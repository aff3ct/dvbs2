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
private:
	std::vector<R> b;
	std::vector<std::complex<R> > buff;
	int head;
	int size;
	int M;
	int P;

public:
	Filter_FIR_ccr (const int N, const std::vector<R> b);
	virtual ~Filter_FIR_ccr();
	inline void step(const std::complex<R>* x_elt, std::complex<R>* y_elt);
	void reset();
	std::vector<R> get_filter_coefs();
protected:
	void _filter(const R *X_N1,  R *Y_N2, const int frame_id);

};

// Adrien: I put this function here because you wanted to be inlined
template <typename R>
void Filter_FIR_ccr<R>
::step(const std::complex<R>* x_elt, std::complex<R>* y_elt)
{
	this->buff[this->head] = *x_elt;
	this->buff[this->head + this->size] = *x_elt;

	std::complex<R> ps = this->buff[this->head+1] * this->b[0];
	for (auto i = 1; i < this->size ; i++)
		ps += this->buff[this->head + 1 + i] * this->b[i];

	*y_elt = ps;

	this->head++;
	this->head %= this->size;
}

}
}

#endif //FILTER_FIR_CCR_HPP

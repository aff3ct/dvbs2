#ifndef FILTER_FARROW_CCR_NAIVE_HPP
#define FILTER_FARROW_CCR_NAIVE_HPP

#include <cassert>
#include <iostream>
#include <complex>

#include "Module/Filter/Filter.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Filter_Farrow_ccr_naive : public Filter<R>
{
private:
	std::complex<R> xnd2_1;
	std::complex<R> xnd2_2;
	std::complex<R> xnd2_3;
	std::complex<R> xn_1;
	std::complex<R> xn_2;

	R mu;
public:
	
	Filter_Farrow_ccr_naive (const int N, const R mu);
	virtual ~Filter_Farrow_ccr_naive();
	void set_mu(R mu);
	void step(const std::complex<R>* x_elt, std::complex<R>* y_elt);
	void _filter(const R *X_N1,  R *Y_N2, const int frame_id);
	void reset();
};
}
}
#endif //FILTER_FARROW_CCR_NAIVE_HPP

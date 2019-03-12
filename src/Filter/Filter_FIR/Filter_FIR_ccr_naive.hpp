#ifndef FILTER_FIR_CCR_NAIVE_HPP
#define FILTER_FIR_CCR_NAIVE_HPP

#include <cassert>
#include <iostream>
#include <vector>
#include <complex>

#include "../Filter.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Filter_FIR_ccr_naive : public Filter<R>
{
private:
	std::vector<R> b;
	std::vector<std::complex<R> > buff;
	int head;
	int size;

public:
	Filter_FIR_ccr_naive (const int N, const std::vector<R> b);
	virtual ~Filter_FIR_ccr_naive();
	inline void step  (const std::complex<R>* x_elt, std::complex<R>* y_elt);
	void reset();
	std::vector<R> get_filter_coefs();
protected:
	void _filter(const R *X_N1,  R *Y_N2, const int frame_id);

};
}
}

#include "Filter_FIR_ccr_naive.hxx"
#endif //FILTER_FIR_CCR_NAIVE_HPP

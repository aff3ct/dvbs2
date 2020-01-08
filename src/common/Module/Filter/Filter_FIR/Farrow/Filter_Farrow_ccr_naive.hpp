#ifndef FILTER_FARROW_CCR_NAIVE_HPP
#define FILTER_FARROW_CCR_NAIVE_HPP

#include <cassert>
#include <iostream>
#include <complex>

#include "Module/Filter/Filter_FIR/Filter_FIR_ccr.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Filter_Farrow_ccr_naive : public Filter_FIR_ccr<R>
{
private:
	R mu;

public:

	Filter_Farrow_ccr_naive (const int N, const R mu, const int n_frames = 1);
	virtual ~Filter_Farrow_ccr_naive();
	void set_mu(R mu);
};
}
}
#endif //FILTER_FARROW_CCR_NAIVE_HPP
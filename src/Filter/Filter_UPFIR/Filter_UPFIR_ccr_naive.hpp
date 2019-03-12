#ifndef FILTER_UPFIR_CCR_NAIVE_HPP
#define FILTER_UPFIR_CCR_NAIVE_HPP

#include <vector>
#include <complex>

#include "../Filter_FIR/Filter_FIR_ccr_naive.hpp"
#include "../Filter.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Filter_UPFIR_ccr_naive : public Filter<R>
{
public:
	Filter_UPFIR_ccr_naive(const int N, const std::vector<R> H, const int F = 1);
	virtual ~Filter_UPFIR_ccr_naive();
	inline void step  (const std::complex<R>* x_elt, std::complex<R>* y_elts);
	void reset();

protected:
	void _filter(const R *X_N1, R *Y_N2, const int frame_id);

private:
	const int                           F;       // Upsampling Factor
	std::vector<std::vector<R> >        H;       // Impulse response
	std::vector<Filter_FIR_ccr_naive<R> > flt_bank;// Filter bank
};
}
}

#endif //FILTER_UPFIR_CCR_NAIVE_HPP

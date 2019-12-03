#ifndef FILTER_UPRRC_CCR_NAIVE_HPP
#define FILTER_UPRRC_CCR_NAIVE_HPP

#include <vector>

#include "Module/Filter/Filter_UPFIR/Filter_UPFIR_ccr_naive.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Filter_UPRRC_ccr_naive : public Filter_UPFIR_ccr_naive<R>
{
private:
	const R   rolloff;
	const int samples_per_symbol;
	const int delay_in_symbol;

	std::vector<R> compute_rrc_coefs(const R rolloff, const int samples_per_symbol, const int delay_in_symbol);

public:
	Filter_UPRRC_ccr_naive (const int N, const R rolloff = 0.05f, const int samples_per_symbol = 4, const int delay_in_symbol = 50, const int n_frames = 1);
	virtual ~Filter_UPRRC_ccr_naive();
};
}
}

#endif //FILTER_UPRRC_CCR_NAIVE_HPP

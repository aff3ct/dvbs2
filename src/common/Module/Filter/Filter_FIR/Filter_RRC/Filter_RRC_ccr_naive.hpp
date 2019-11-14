#ifndef FILTER_RRC_CCR_NAIVE_HPP
#define FILTER_RRC_CCR_NAIVE_HPP

#include <vector>

#include "Module/Filter/Filter_FIR/Filter_FIR_ccr.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Filter_RRC_ccr_naive : public Filter_FIR_ccr<R>
{
private:
	const R   rolloff;
	const int samples_per_symbol;
	const int delay_in_symbol;

	std::vector<R> compute_rrc_coefs(const R rolloff, const int samples_per_symbol, const int delay_in_symbol);

public:
	Filter_RRC_ccr_naive (const int N, const R rolloff = 0.05f, const int samples_per_symbol = 4, const int delay_in_symbol = 50);
	virtual ~Filter_RRC_ccr_naive();
};
}
}

#endif //FILTER_RRC_CCR_NAIVE_HPP

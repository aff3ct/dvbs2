/*!
 * \file
 * \brief Class tools::Reporter_noise_DVBS2O.
 */
#ifndef REPORTER_NOISE_DVBS2O_HPP_
#define REPORTER_NOISE_DVBS2O_HPP_

#include "Tools/Noise/Noise.hpp"
#include "Tools/Display/Reporter/Reporter.hpp"

namespace aff3ct
{
namespace tools
{
template <typename R = float>
class Reporter_noise_DVBS2O : public Reporter
{
protected:
	const Noise<R>& noise_estimated;
	const Noise<R>& noise;
	const bool show_ground_truth;
	group_t noise_group;
	report_t final_report;

	R Eb_N0_avg;
	R alpha;

public:
	explicit Reporter_noise_DVBS2O(const Noise<R>& noise_estimated, const Noise<R>& noise, const bool show_ground_truth = false, R alpha = 0.9);
	virtual ~Reporter_noise_DVBS2O() = default;

	report_t report(bool final = false);
};
}
}

#endif /* REPORTER_NOISE_DVBS2O_HPP_ */

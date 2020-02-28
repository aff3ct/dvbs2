/*!
 * \file
 * \brief Class tools::Reporter_noise_DVBS2.
 */
#ifndef REPORTER_NOISE_DVBS2_HPP_
#define REPORTER_NOISE_DVBS2_HPP_

#include "Tools/Noise/Noise.hpp"
#include "Tools/Display/Reporter/Reporter.hpp"

namespace aff3ct
{
namespace tools
{
template <typename R = float>
class Reporter_noise_DVBS2 : public Reporter
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
	explicit Reporter_noise_DVBS2(const Noise<R>& noise_estimated, const Noise<R>& noise, const bool show_ground_truth = false, R alpha = 0.9);
	virtual ~Reporter_noise_DVBS2() = default;

	report_t report(bool final = false);
};
}
}

#endif /* REPORTER_NOISE_DVBS2_HPP_ */

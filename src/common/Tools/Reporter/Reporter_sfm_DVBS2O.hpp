/*!
 * \file
 * \brief Class tools::Reporter_sfm_DVBS2O.
 */
#ifndef REPORTER_SFM_DVBS2O_HPP_
#define REPORTER_SFM_DVBS2O_HPP_

#include "aff3ct.hpp"
#include "Reporter_buffered.hpp"
#include "Module/Probe/Probe.hpp"
#include "Module/Synchronizer/Synchronizer_frame/Synchronizer_frame.hpp"


namespace aff3ct
{
namespace tools
{
template <typename B = int, typename R = float>
class Reporter_sfm_DVBS2O : public Reporter_buffered<R>
{
private:
	const module::Synchronizer_frame<R>       &sfm;
	Reporter::group_t synchro_group;
	Reporter::report_t final_report;

public:
	explicit Reporter_sfm_DVBS2O(const module::Synchronizer_frame<R>       &sfm,
	                             const int max_size = 10000);

	virtual ~Reporter_sfm_DVBS2O() = default;

	Reporter::report_t report(bool final = false);

	virtual void _probe(std::string col_name);
	void create_groups();

	module::Probe<R>* build_probe();
};
}
}

#endif /* REPORTER_DVBS2OP_ */

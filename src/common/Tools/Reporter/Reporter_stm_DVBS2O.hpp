/*!
 * \file
 * \brief Class tools::Reporter_stm_DVBS2O.
 */
#ifndef REPORTER_STM_DVBS2O_HPP_
#define REPORTER_STM_DVBS2O_HPP_

#include "aff3ct.hpp"
#include "Reporter_buffered.hpp"
#include "Module/Probe/Probe.hpp"

#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_timing.hpp"


namespace aff3ct
{
namespace tools
{
template <typename B = int, typename R = float>
class Reporter_stm_DVBS2O : public Reporter_buffered<R>
{
private:
	const module::Synchronizer_timing<B,R>    &stm;
	Reporter::group_t synchro_group;
	Reporter::report_t final_report;

public:
	explicit Reporter_stm_DVBS2O(const module::Synchronizer_timing<B,R>    &stm,
	                             const int max_size = 10000);

	virtual ~Reporter_stm_DVBS2O() = default;

	Reporter::report_t report(bool final = false);

	virtual void _probe(std::string col_name);
	void create_groups();

	module::Probe<R>* build_probe();
};
}
}

#endif /* REPORTER_DVBS2OP_ */

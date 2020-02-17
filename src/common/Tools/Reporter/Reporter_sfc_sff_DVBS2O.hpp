/*!
 * \file
 * \brief Class tools::Reporter_sfc_sff_DVBS2O.
 */
#ifndef REPORTER_DVBS2O_HPP_
#define REPORTER_DVBS2O_HPP_

#include "aff3ct.hpp"
#include "Reporter_buffered.hpp"
#include "Module/Probe/Probe.hpp"
#include "Module/Synchronizer/Synchronizer_freq/Synchronizer_freq_coarse/Synchronizer_freq_coarse.hpp"
#include "Module/Synchronizer/Synchronizer_freq/Synchronizer_freq_fine/Synchronizer_freq_fine.hpp"
#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_timing.hpp"
#include "Module/Synchronizer/Synchronizer_frame/Synchronizer_frame.hpp"


namespace aff3ct
{
namespace tools
{
template <typename B = int, typename R = float>
class Reporter_sfc_sff_DVBS2O : public Reporter_buffered<R>
{
private:
	const module::Synchronizer_freq_coarse<R> &sfc;
	const module::Synchronizer_freq_fine<R>   &sff;
	const module::Synchronizer_freq_fine<R>   &spf;
	Reporter::group_t synchro_group;
	const int osf;
	Reporter::report_t final_report;

public:
	explicit Reporter_sfc_sff_DVBS2O(const module::Synchronizer_freq_coarse<R> &sfc,
	                                 const module::Synchronizer_freq_fine<R>   &sff,
	                                 const module::Synchronizer_freq_fine<R>   &spf,
	                                 const int osf = 1,
	                                 const int max_size = 10000);

	virtual ~Reporter_sfc_sff_DVBS2O() = default;

	Reporter::report_t report(bool final = false);

	virtual void _probe(std::string col_name);
	void create_groups();

	module::Probe<R>* build_sfc_probe();
	module::Probe<R>* build_sff_probe();
	module::Probe<R>* build_spf_probe();
};
}
}

#endif /* REPORTER_DVBS2OP_ */

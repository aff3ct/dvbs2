/*!
 * \file
 * \brief Class tools::Reporter_buffered_sync.
 */
#ifndef REPORTER_BUFFERED_SYNC_HPP_
#define REPORTER_BUFFERED_SYNC_HPP_

#include "aff3ct.hpp"
#include "Reporter_buffered.hpp"
#include "Module/Synchronizer/Synchronizer_freq/Synchronizer_freq_coarse/Synchronizer_freq_coarse.hpp"
#include "Module/Synchronizer/Synchronizer_freq/Synchronizer_freq_fine/Synchronizer_freq_fine.hpp"
#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_timing.hpp"
#include "Module/Synchronizer/Synchronizer_frame/Synchronizer_frame.hpp"


namespace aff3ct
{
namespace tools
{
template <typename R = float>
class Reporter_buffered_sync : public Reporter_buffered<R>
{
private:
	const module::Synchronizer_freq_coarse<R> &sfc;
	const module::Synchronizer_timing<R>      &stm;
	const module::Synchronizer_frame<R>       &sfm;
	const module::Synchronizer_freq_fine<R>   &sff;
	const module::Synchronizer_freq_fine<R>   &sfp;

public:
	explicit Reporter_buffered_sync(const module::Synchronizer_freq_coarse<R> &sfc,
	                                const module::Synchronizer_timing<R>      &stm,
	                                const module::Synchronizer_frame<R>       &sfm,
	                                const module::Synchronizer_freq_fine<R>   &sff,
	                                const module::Synchronizer_freq_fine<R>   &sfp,
	                                const int max_size);

	virtual ~Reporter_buffered_sync() = default;

	Reporter::report_t report(bool final = false);

	void probe(std::string col_name);
};
}
}

#endif /* REPORTER_BUFFERED_SYNC_HPP_ */

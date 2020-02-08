/*!
 * \file
 * \brief Class tools::Reporter_throughput_dvbs2o.
 */
#ifndef REPORTER_THROUGHPUT_DVBS2O_HPP_
#define REPORTER_THROUGHPUT_DVBS2O_HPP_

#include <chrono>
#include <cstdint>
#include <functional>
#include <type_traits>

#include "Module/Monitor/MI/Monitor_MI.hpp"
#include "Module/Monitor/BFER/Monitor_BFER.hpp"
#include "Module/Monitor/EXIT/Monitor_EXIT.hpp"
#include "Tools/Display/Reporter/Reporter.hpp"

namespace aff3ct
{
namespace tools
{
template <typename T = uint64_t>
class Reporter_throughput_dvbs2o : public Reporter
{
	static_assert(std::is_convertible<T, double>::value, "T type must be convertible to a double.");

protected:
	std::function<T(void)> progress_function;
	std::function<T(void)> get_nbits_function;

	const T progress_limit;
	const T nbits_factor;

	std::chrono::time_point<std::chrono::steady_clock> t_report;
	std::chrono::time_point<std::chrono::steady_clock> t_prev_report;

	group_t throughput_group;

public:
	explicit Reporter_throughput_dvbs2o(std::function<T(void)> progress_function, const T progress_limit = 0,
	                             std::function<T(void)> get_nbits_function = nullptr, const T nbits_factor = 1);

	template<typename B>
	explicit Reporter_throughput_dvbs2o(const module::Monitor_BFER<B>& m);

	template<typename B, typename R>
	explicit Reporter_throughput_dvbs2o(const module::Monitor_MI<B,R>& m);

	template<typename B, typename R>
	explicit Reporter_throughput_dvbs2o(const module::Monitor_EXIT<B,R>& m);

	virtual ~Reporter_throughput_dvbs2o() = default;

	report_t report(bool final = false);

	void init();
};
}
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include "Reporter_throughput_dvbs2o.hxx"
#endif

#endif /* Reporter_throughput_dvbs2o_HPP_ */

/*!
 * \file
 * \brief Class tools::Reporter_throughput_DVBS2.
 */
#ifndef Reporter_throughput_DVBS2_HPP_
#define Reporter_throughput_DVBS2_HPP_

#include <chrono>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <streampu.hpp>

#include "Module/Monitor/MI/Monitor_MI.hpp"
#include "Module/Monitor/BFER/Monitor_BFER.hpp"
#include "Module/Monitor/EXIT/Monitor_EXIT.hpp"

namespace aff3ct
{
namespace tools
{
template <typename T = uint64_t>
class Reporter_throughput_DVBS2 : public spu::tools::Reporter
{
	static_assert(std::is_convertible<T, double>::value, "T type must be convertible to a double.");

protected:
	std::function<T(void)> progress_function;
	std::function<T(void)> get_nbits_function;

	const T progress_limit;
	const T nbits_factor;
	const T n_frames;
	double alpha;
	double tpt_mem;
	std::chrono::time_point<std::chrono::steady_clock> t_report;
	std::chrono::time_point<std::chrono::steady_clock> t_prev_report;

	group_t throughput_group;
	Reporter::report_t final_report;


public:
	explicit Reporter_throughput_DVBS2(std::function<T(void)> progress_function, const T progress_limit = 0,
	                             std::function<T(void)> get_nbits_function = nullptr, const T nbits_factor = 1, T n_frames = 1, double alpha = 0.9);

	template<typename B>
	explicit Reporter_throughput_DVBS2(const module::Monitor_BFER<B>& m, double alpha = 0.9);

	template<typename B, typename R>
	explicit Reporter_throughput_DVBS2(const module::Monitor_MI<B,R>& m, double alpha = 0.9);

	template<typename B, typename R>
	explicit Reporter_throughput_DVBS2(const module::Monitor_EXIT<B,R>& m, double alpha = 0.9);

	virtual ~Reporter_throughput_DVBS2() = default;

	spu::tools::Reporter::report_t report(bool final = false);

	void init();
};
}
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include "Reporter_throughput_DVBS2.hxx"
#endif

#endif /* Reporter_throughput_DVBS2_HPP_ */

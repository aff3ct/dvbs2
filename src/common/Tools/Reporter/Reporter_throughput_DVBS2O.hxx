#include <sstream>
#include <iomanip>
#include <utility>
#include <ios>

#include "Tools/general_utils.h"
#include "Reporter_throughput_DVBS2O.hpp"

namespace aff3ct
{
namespace tools
{
template <typename T>
Reporter_throughput_DVBS2O<T>
::Reporter_throughput_DVBS2O(std::function<T(void)>  progress_function, const T progress_limit,
                      std::function<T(void)> get_nbits_function, const T nbits_factor, const T n_frames, double alpha)
: Reporter(),
  progress_function(progress_function),
  get_nbits_function(get_nbits_function),
  progress_limit(progress_limit),
  nbits_factor(nbits_factor),
  n_frames(n_frames),
  alpha(alpha),
  tpt_mem(0.0),
  t_report(std::chrono::steady_clock::now()),
  t_prev_report(std::chrono::steady_clock::now())
{
	auto& throughput_title = throughput_group.first;
	auto& throughput_cols  = throughput_group.second;

	throughput_title = std::make_pair("Throughput", "and elapsed time");

	throughput_cols.push_back(std::make_pair("CUR_THR", "(Mb/s)"));
	throughput_cols.push_back(std::make_pair("AVG_THR", "(Mb/s)"));
	throughput_cols.push_back(std::make_pair("ET/RT", "(hhmmss)"));

	this->cols_groups.push_back(throughput_group);
}

template <typename T>
template <typename B>
Reporter_throughput_DVBS2O<T>
::Reporter_throughput_DVBS2O(const module::Monitor_BFER<B>& m, double alpha)
: Reporter_throughput_DVBS2O(std::bind(&module::Monitor_BFER<B>::get_n_fe, &m),
	                  (T)m.get_max_fe(),
	                  std::bind(&module::Monitor_BFER<B>::get_n_analyzed_fra, &m),
	                  (T)m.get_K(),
					  (T)m.get_n_frames(),
					  alpha)
{
}

template <typename T>
template <typename B, typename R>
Reporter_throughput_DVBS2O<T>
::Reporter_throughput_DVBS2O(const module::Monitor_MI<B,R>& m, double alpha)
: Reporter_throughput_DVBS2O(std::bind(&module::Monitor_MI<B,R>::get_n_trials, &m),
	                  (T)m.get_max_n_trials(),
	                  std::bind(&module::Monitor_MI<B,R>::get_n_trials, &m),
	                  (T)m.get_N(), (T)m.get_n_frames(), alpha)
{
}

template <typename T>
template <typename B, typename R>
Reporter_throughput_DVBS2O<T>
::Reporter_throughput_DVBS2O(const module::Monitor_EXIT<B,R>& m, double alpha)
: Reporter_throughput_DVBS2O(std::bind(&module::Monitor_EXIT<B,R>::get_n_trials, &m),
	                  (T)m.get_max_n_trials(),
	                  std::bind(&module::Monitor_EXIT<B,R>::get_n_trials, &m),
	                  (T)m.get_N(), (T)m.get_n_frames(), alpha)
{
}

template <typename T>
Reporter::report_t Reporter_throughput_DVBS2O<T>
::report(bool final)
{
	if (final)
	{
		auto old_report = this->final_report;
		init();
		return old_report;
	}

	assert(this->cols_groups.size() == 1);

	report_t report(this->cols_groups.size());

	auto& thgput_report = report[0];

	T progress = 0/*, nbits = 0*/;

	if (progress_function != nullptr)
		progress = progress_function();

	// if (get_nbits_function != nullptr)
	// 	nbits = get_nbits_function() * nbits_factor;

	using namespace std::chrono;
	auto now       = steady_clock::now();
	auto simu_time = (double)duration_cast<microseconds>(now - t_report     ).count(); // usec
	auto delta     = (double)duration_cast<microseconds>(now - t_prev_report).count(); // usec
	t_prev_report  = now;
	double displayed_time  = simu_time * 1e-6; // sec
	double displayed_delta = delta     * 1e-6; // sec

	if (!final && progress != 0 && progress_limit != 0)
		displayed_time *= (double)progress_limit / (double)progress - 1.;

	if (!final && progress != 0 && progress_limit != 0)
		displayed_delta *= (double)progress_limit / (double)progress - 1.;

	auto str_time = get_time_format(displayed_time);
	//auto simu_thr = (double)nbits / simu_time; // = Mbps

	auto str_delay = get_time_format(delta);
	auto curr_thr = (double)(nbits_factor*n_frames) / delta; // = Mbps
	this->tpt_mem = this->alpha*this->tpt_mem + (double)curr_thr; // = Mbps
	auto simu_thr = this->tpt_mem * ((double)1-this->alpha);
	std::stringstream str_dlt;
	str_dlt << std::setprecision(3) << std::fixed << curr_thr;

	std::stringstream str_thr;
	str_thr << std::setprecision(3) << std::fixed << simu_thr;

	thgput_report.push_back(str_dlt.str());
	thgput_report.push_back(str_thr.str());
	thgput_report.push_back(str_time);

	this->final_report = report;
	return report;
}

template <typename T>
void Reporter_throughput_DVBS2O<T>
::init()
{
	Reporter::init();

	t_report = std::chrono::steady_clock::now();
	t_prev_report = std::chrono::steady_clock::now();

	this->final_report.clear();
}
}
}

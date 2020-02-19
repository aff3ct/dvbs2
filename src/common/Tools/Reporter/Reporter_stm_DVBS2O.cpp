#include <utility>
#include <iomanip>
#include <sstream>
#include <cassert>
#include <ios>

#include "Reporter_stm_DVBS2O.hpp"

using namespace aff3ct;
using namespace aff3ct::tools;


template <typename B, typename R>
Reporter_stm_DVBS2O<B,R>
::Reporter_stm_DVBS2O(const module::Synchronizer_timing<B,R> &stm,
	                  const int max_size)
: Reporter_buffered<R>(1, max_size),
stm(stm)
{
	this->column_keys["DEL"] = 0;
	create_groups();
}

template <typename B, typename R>
void Reporter_stm_DVBS2O<B,R>
::create_groups()
{

	auto& title = this->synchro_group.first;
	auto& cols  = this->synchro_group.second;

	title = {"Timing Synchronization data", "Gardner Alg."};

	cols.push_back(std::make_pair("DEL", "FRAC"));

	this->cols_groups.push_back(this->synchro_group);
}

template <typename B, typename R>
void Reporter_stm_DVBS2O<B,R>
::_probe(std::string col_name)
{
	R sampled_value = 0.0f;
	sampled_value = stm.get_mu();
	this->push(&sampled_value, "DEL");
	//this->print_buffer();
}

template <typename B, typename R>
Reporter::report_t Reporter_stm_DVBS2O<B,R>
::report(bool final)
{
	if (final)
		return this->final_report;

	assert(this->cols_groups.size() == 1);
	Reporter::report_t the_report(this->cols_groups.size());
	auto& sync_report = the_report[0];

	std::stringstream stream;
	R buffer_content = 0.0f;
	this->pull(&buffer_content, 1, 0);
	stream << std::setprecision(3) << std::scientific << buffer_content;
	sync_report.push_back(stream.str());
	this->final_report = the_report;
	return the_report;
}

template <typename B, typename R>
module::Probe<R>* Reporter_stm_DVBS2O<B,R>
::build_probe()
{
	return new module::Probe<R>(this->stm.get_N_in(), "", *this, this->stm.get_n_frames());
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
#ifdef AFF3CT_MULTI_PREC
template class aff3ct::tools::Reporter_stm_DVBS2O<B_32, R_32>;
template class aff3ct::tools::Reporter_stm_DVBS2O<B_64, R_64>;
#else
template class aff3ct::tools::Reporter_stm_DVBS2O<B,R>;
#endif
// ==================================================================================== explicit template instantiation

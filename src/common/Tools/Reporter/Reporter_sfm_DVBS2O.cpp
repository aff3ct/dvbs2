#include <utility>
#include <iomanip>
#include <sstream>
#include <cassert>
#include <ios>

#include "Reporter_sfm_DVBS2O.hpp"

using namespace aff3ct;
using namespace aff3ct::tools;


template <typename B, typename R>
Reporter_sfm_DVBS2O<B,R>
::Reporter_sfm_DVBS2O(const module::Synchronizer_frame<R> &sfm,
	                  const int max_size)
: Reporter_buffered<R>(3, max_size),
sfm(sfm)
{
	this->column_keys["DEL"] = 0;
	this->column_keys["FLG"] = 1;
	this->column_keys["TRI"] = 2;

	create_groups();
}

template <typename B, typename R>
void Reporter_sfm_DVBS2O<B,R>
::create_groups()
{

	auto& title = this->synchro_group.first;
	auto& cols  = this->synchro_group.second;

	title = {"Frame Synchronization", ""};
	cols.push_back(std::make_pair("DEL", ""));
	cols.push_back(std::make_pair("FLG", ""));
	cols.push_back(std::make_pair("TRI", ""));

	this->cols_groups.push_back(this->synchro_group);
}

template <typename B, typename R>
void Reporter_sfm_DVBS2O<B,R>
::_probe(std::string col_name)
{
	R sampled_value = 0.0f;
	sampled_value = (R)sfm.get_delay();
	this->push(&sampled_value, "DEL");

	sampled_value = (R)sfm.get_packet_flag();
	this->push(&sampled_value, "FLG");

	sampled_value = (R)sfm.get_metric();
	this->push(&sampled_value, "TRI");
}

template <typename B, typename R>
Reporter::report_t Reporter_sfm_DVBS2O<B,R>
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
	stream << (int)buffer_content;
	sync_report.push_back(stream.str());

	stream.str("");
	this->pull(&buffer_content, 1, 1);
	stream << (int)buffer_content;
	sync_report.push_back(stream.str());

	stream.str("");
	this->pull(&buffer_content, 1, 2);
	stream << std::setprecision(3) << std::scientific << buffer_content;
	sync_report.push_back(stream.str());
	this->final_report = the_report;
	return the_report;
}


template <typename B, typename R>
module::Probe<R>* Reporter_sfm_DVBS2O<B,R>
::build_probe()
{
	return new module::Probe<R>(this->sfm.get_N_in(), "", *this, this->sfm.get_n_frames());
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
#ifdef AFF3CT_MULTI_PREC
template class aff3ct::tools::Reporter_sfm_DVBS2O<B_32, R_32>;
template class aff3ct::tools::Reporter_sfm_DVBS2O<B_64, R_64>;
#else
template class aff3ct::tools::Reporter_sfm_DVBS2O<B,R>;
#endif
// ==================================================================================== explicit template instantiation

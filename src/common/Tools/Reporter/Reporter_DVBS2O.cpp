#include <utility>
#include <iomanip>
#include <sstream>
#include <cassert>
#include <ios>

#include "Reporter_DVBS2O.hpp"

using namespace aff3ct;
using namespace aff3ct::tools;


template <typename B, typename R>
Reporter_DVBS2O<B,R>
::Reporter_DVBS2O(const module::Synchronizer_freq_coarse<R> &sfc,
	              const module::Synchronizer_timing<B,R>    &stm,
	              const module::Synchronizer_frame<R>       &sfm,
	              const module::Synchronizer_freq_fine<R>   &sff,
	              const int max_size)
: Reporter_buffered<R>(4, max_size),
sfc(sfc), stm(stm), sfm(sfm), sff(sff)
{
	this->column_keys["sfm"] = 0;
	this->column_keys["stm"] = 1;
	this->column_keys["sfc"] = 2;
	this->column_keys["sff"] = 3;

	create_groups();
}

template <typename B, typename R>
void Reporter_DVBS2O<B,R>
::create_groups()
{

	auto& title = this->synchro_group.first;
	auto& cols  = this->synchro_group.second;

	title = {"Synchronization data", ""};
	cols.push_back(std::make_pair("SFM", ""));
	cols.push_back(std::make_pair("STM", ""));
	cols.push_back(std::make_pair("SFC", ""));
	cols.push_back(std::make_pair("SFF", ""));

	this->cols_groups.push_back(this->synchro_group);
}

template <typename B, typename R>
void Reporter_DVBS2O<B,R>
::_probe(std::string col_name)
{
	R sampled_value = 0.0f;

	if (col_name == "sfc")
		sampled_value = this->sfc.get_estimated_freq();
	else if (col_name == "stm")
		sampled_value = stm.get_mu();
	else if (col_name == "sfm")
		sampled_value = (R)sfm.get_delay();
	else if (col_name == "sff")
		sampled_value = sff.get_estimated_freq();

	this->push(&sampled_value, col_name);
	//this->print_buffer();
}

template <typename B, typename R>
Reporter::report_t Reporter_DVBS2O<B,R>
::report(bool final)
{
	assert(this->cols_groups.size() == 1);
	Reporter::report_t the_report(this->cols_groups.size());
	auto& sync_report = the_report[0];

	for (int c = 0 ; c < this->buffer.size(); c++)
	{
		std::stringstream stream;
		R buffer_content = 0.0f;
		this->pull(&buffer_content, 1, c);
		if (c > 0)
		{
			std::stringstream local_stream;
			if (buffer_content >= 0)
				local_stream << std::setprecision(3) << std::scientific << " " << buffer_content;
			else
				local_stream << std::setprecision(3) << std::scientific << buffer_content;
			stream << std::setprecision(4) << local_stream.str();
		}
		else
			stream << (int)buffer_content;

		sync_report.push_back(stream.str());
	}



	return the_report;
}

template <typename B, typename R>
module::Probe<R>* Reporter_DVBS2O<B,R>
::build_sfm_probe()
{
	return new module::Probe<R>(this->sfm.get_N_out(), "sfm", *this, this->sfm.get_n_frames());
}

template <typename B, typename R>
module::Probe<R>* Reporter_DVBS2O<B,R>
::build_stm_probe()
{
	return new module::Probe<R>(this->stm.get_N_in(), "stm", *this, this->stm.get_n_frames());
}

template <typename B, typename R>
module::Probe<R>* Reporter_DVBS2O<B,R>
::build_sff_probe()
{
	return new module::Probe<R>(this->sff.get_N(), "sff", *this, this->sff.get_n_frames());
}

template <typename B, typename R>
module::Probe<R>* Reporter_DVBS2O<B,R>
::build_sfc_probe()
{
	return new module::Probe<R>(this->sfc.get_N(), "sfc", *this, this->sfc.get_n_frames());
}
// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
#ifdef AFF3CT_MULTI_PREC
template class aff3ct::tools::Reporter_DVBS2O<B_32, R_32>;
template class aff3ct::tools::Reporter_DVBS2O<B_64, R_64>;
#else
template class aff3ct::tools::Reporter_DVBS2O<B,R>;
#endif
// ==================================================================================== explicit template instantiation

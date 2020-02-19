#include <utility>
#include <iomanip>
#include <sstream>
#include <cassert>
#include <ios>

#include "Reporter_sfc_sff_DVBS2O.hpp"

using namespace aff3ct;
using namespace aff3ct::tools;


template <typename B, typename R>
Reporter_sfc_sff_DVBS2O<B,R>
::Reporter_sfc_sff_DVBS2O(const module::Synchronizer_freq_coarse<R> &sfc,
	                      const module::Synchronizer_freq_fine<R>   &sff,
	                      const module::Synchronizer_freq_fine<R>   &spf,
	                      const int osf,
	                      const int max_size)
: Reporter_buffered<R>(3, max_size),
sfc(sfc), sff(sff), spf(spf), osf(osf)
{
	this->column_keys["sfc"] = 0;
	this->column_keys["sff"] = 1;
	this->column_keys["spf"] = 2;

	create_groups();
}

template <typename B, typename R>
void Reporter_sfc_sff_DVBS2O<B,R>
::create_groups()
{

	auto& title = this->synchro_group.first;
	auto& cols  = this->synchro_group.second;

	title = {"Freq. Synchronization", ""};
	cols.push_back(std::make_pair("COA", "CFO"));
	cols.push_back(std::make_pair("L&R", "CFO"));
	cols.push_back(std::make_pair("FIN", "CFO"));

	this->cols_groups.push_back(this->synchro_group);
}

template <typename B, typename R>
void Reporter_sfc_sff_DVBS2O<B,R>
::_probe(std::string col_name)
{
	if (col_name == "sfc")
	{
		R sampled_value = 0.0f;
		sampled_value = this->sfc.get_estimated_freq();
		this->push(&sampled_value, col_name);
	}
	else if (col_name == "sff")
	{
		R sampled_value = 0.0f;
		sampled_value = sff.get_estimated_freq();
		this->push(&sampled_value, col_name);
	}
	else if (col_name == "spf")
	{
		R sampled_value = 0.0f;
		sampled_value = spf.get_estimated_freq();
		this->push(&sampled_value, col_name);
	}
	//this->print_buffer();
}

template <typename B, typename R>
Reporter::report_t Reporter_sfc_sff_DVBS2O<B,R>
::report(bool final)
{
	if (final)
	{
		return this->final_report;
	}
	assert(this->cols_groups.size() == 1);
	Reporter::report_t the_report(this->cols_groups.size());
	auto& sync_report = the_report[0];

	std::stringstream stream;
	std::stringstream temp_stream;

	R cfo = 0.0f;
	this->pull(&cfo, 1, 0);
	if (cfo >= 0)
		temp_stream << std::setprecision(3) << std::scientific << " " << cfo;
	else
		temp_stream << std::setprecision(3) << std::scientific << cfo;

	stream << std::setprecision(4) << temp_stream.str();
	sync_report.push_back(stream.str());


	R lr_cfo = 0.0f;
	temp_stream.str("");
	stream.str("");
	this->pull(&lr_cfo, 1, 1);
	lr_cfo /= (R)this->osf;
	cfo = lr_cfo;
	if (cfo >= 0)
		temp_stream << std::setprecision(3) << std::scientific << " " << cfo;
	else
		temp_stream << std::setprecision(3) << std::scientific << cfo;

	stream << std::setprecision(4) << temp_stream.str();
	sync_report.push_back(stream.str());


	R ff_cfo = 0.0f;
	temp_stream.str("");
	stream.str("");
	this->pull(&ff_cfo, 1, 2);
	ff_cfo /= (R)this->osf;
	cfo = ff_cfo;
	if (cfo >= 0)
		temp_stream << std::setprecision(3) << std::scientific << " " << cfo;
	else
		temp_stream << std::setprecision(3) << std::scientific << cfo;

	stream << std::setprecision(4) << temp_stream.str();
	sync_report.push_back(stream.str());
	this->final_report = the_report;
	return the_report;
}

template <typename B, typename R>
module::Probe<R>* Reporter_sfc_sff_DVBS2O<B,R>
::build_sff_probe()
{
	return new module::Probe<R>(this->sff.get_N(), "sff", *this, this->sff.get_n_frames());
}

template <typename B, typename R>
module::Probe<R>* Reporter_sfc_sff_DVBS2O<B,R>
::build_spf_probe()
{
	return new module::Probe<R>(this->spf.get_N(), "spf", *this, this->spf.get_n_frames());
}


template <typename B, typename R>
module::Probe<R>* Reporter_sfc_sff_DVBS2O<B,R>
::build_sfc_probe()
{
	return new module::Probe<R>(this->sfc.get_N(), "sfc", *this, this->sfc.get_n_frames());
}
// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
#ifdef AFF3CT_MULTI_PREC
template class aff3ct::tools::Reporter_sfc_sff_DVBS2O<B_32, R_32>;
template class aff3ct::tools::Reporter_sfc_sff_DVBS2O<B_64, R_64>;
#else
template class aff3ct::tools::Reporter_sfc_sff_DVBS2O<B,R>;
#endif
// ==================================================================================== explicit template instantiation

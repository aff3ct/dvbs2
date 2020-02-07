#include <utility>
#include <iomanip>
#include <sstream>
#include <cassert>
#include <ios>

#include "Reporter_buffered_sync.hpp"

using namespace aff3ct;
using namespace aff3ct::tools;


template <typename R>
Reporter_buffered_sync<R>
::Reporter_buffered_sync(const module::Synchronizer_freq_coarse<R> &sfc,
	                     const module::Synchronizer_timing<R>      &stm,
	                     const module::Synchronizer_frame<R>       &sfm,
	                     const module::Synchronizer_freq_fine<R>   &sff,
	                     const module::Synchronizer_freq_fine<R>   &sfp,
	                     const int max_size)
: Reporter_buffered<R>(5, max_size),
sfc(sfc), stm(stm), sfm(sfm), sff(sff), sfp(sfp)
{
	this->column_keys["sfc"] = 0;
	this->column_keys["stm"] = 1;
	this->column_keys["sfm"] = 2;
	this->column_keys["sff"] = 3;
	this->column_keys["sfp"] = 4;
}
template <typename R>
void Reporter_buffered_sync<R>
::probe(std::string col_name);
{
	R sampled_value;
	if (col_name == "sfc")
		sampled_value = sfc.get_estimated_freq ()

	else if (col_name == "stm")
		sampled_value = stm.get_mu();

	else if (col_name == "sfm")
		sampled_value = sfm.get_mu();

	this->push(sampled_value, col_name);
}

template <typename R>
Reporter::report_t Reporter_buffered_sync<R>
::report(bool final)
{
	assert(this->cols_groups.size() == 1);
	Reporter::report_t the_report(this->cols_groups.size());
	auto& sync_report = the_report[0];
	std::stringstream stream;

	for (int c = 0 ; c < this->buffer.size(); c++)
	{
		R buffer_content;
		this->pull(&buffer_content, c, 1);
		stream << std::setprecision(4) << std::fixed << buffer_content;
	}

	sync_report.push_back(stream.str());

	return the_report;
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
#ifdef AFF3CT_MULTI_PREC
template class aff3ct::tools::Reporter_buffered_sync<R_32>;
template class aff3ct::tools::Reporter_buffered_sync<R_64>;
#else
template class aff3ct::tools::Reporter_buffered_sync<R>;
#endif
// ==================================================================================== explicit template instantiation

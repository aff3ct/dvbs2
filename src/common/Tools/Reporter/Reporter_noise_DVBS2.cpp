#include <utility>
#include <tuple>
#include <iomanip>
#include <sstream>
#include <cassert>
#include <ios>
#include <iostream>

#include "Tools/Noise/Sigma.hpp"
#include "Reporter_noise_DVBS2.hpp"

using namespace aff3ct;
using namespace aff3ct::tools;

template <typename R>
Reporter_noise_DVBS2<R>
::Reporter_noise_DVBS2(const Noise<R>& noise_estimated, const Noise<R>& noise, const bool show_ground_truth, R alpha)
: spu::tools::Reporter(),
  noise_estimated(noise_estimated),
  noise(noise),
  show_ground_truth(show_ground_truth),
  Eb_N0_avg ((R)0),
  alpha(alpha)
{
	auto& Noise_title = noise_group.first;
	auto& Noise_cols  = noise_group.second;

	Noise_title = {"Signal Noise Ratio", "(SNR)", 0};
	Noise_cols.push_back(std::make_tuple("Es/N0", "(dB)", 0));
	Noise_cols.push_back(std::make_tuple("Eb/N0", "(dB)", 0));
	Noise_cols.push_back(std::make_tuple("AVG Eb/N0", "(dB)", 0));
	if (show_ground_truth)
	{
		Noise_cols.push_back(std::make_tuple("GT Es/N0", "(dB)", 0));
		Noise_cols.push_back(std::make_tuple("GT Eb/N0", "(dB)", 0));
	}

	this->cols_groups.push_back(noise_group);
}

template <typename R>
spu::tools::Reporter::report_t Reporter_noise_DVBS2<R>
::report(bool final)
{
	assert(this->cols_groups.size() == 1);

	report_t the_report(this->cols_groups.size());

	auto& noise_report = the_report[0];

	std::stringstream stream;

	auto sig = dynamic_cast<const tools::Sigma<R>*>(&noise_estimated);
	try
	{
		stream << std::setprecision(2) << std::fixed << sig->get_esn0();
		noise_report.push_back(stream.str());

		stream.str("");
		stream << std::setprecision(2) << std::fixed << sig->get_ebn0();
		noise_report.push_back(stream.str());

		R Eb_N0_cur = sig->get_ebn0();

		this->Eb_N0_avg = this->Eb_N0_avg * this->alpha + Eb_N0_cur * (1-this->alpha);

		stream.str("");
		stream << std::setprecision(2) << std::fixed << this->Eb_N0_avg;
		noise_report.push_back(stream.str());
	}
	catch(spu::tools::runtime_error const& e)
	{
		stream.str("-");

		noise_report.push_back(stream.str());
		noise_report.push_back(stream.str());
		noise_report.push_back(stream.str());
	}

	if (this->show_ground_truth)
	{
		stream.str("");
		auto sig_gt = dynamic_cast<const tools::Sigma<R>*>(&noise);
		stream << std::setprecision(2) << std::fixed << sig_gt->get_esn0();
		noise_report.push_back(stream.str());

		stream.str("");
		stream << std::setprecision(2) << std::fixed << sig_gt->get_ebn0();
		noise_report.push_back(stream.str());
	}
	return the_report;
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
#ifdef AFF3CT_MULTI_PREC
template class aff3ct::tools::Reporter_noise_DVBS2<R_32>;
template class aff3ct::tools::Reporter_noise_DVBS2<R_64>;
#else
template class aff3ct::tools::Reporter_noise_DVBS2<R>;
#endif
// ==================================================================================== explicit template instantiation

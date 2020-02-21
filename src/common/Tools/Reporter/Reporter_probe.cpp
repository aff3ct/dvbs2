#include <utility>
#include <iomanip>
#include <sstream>
#include <cassert>
#include <ios>

#include "Reporter_probe.hpp"

using namespace aff3ct;
using namespace aff3ct::tools;

Reporter_probe
::Reporter_probe(const std::string &group_name, const std::string &group_description, const int n_frames)
: Reporter(),
  n_frames(n_frames),
  mtx()
{
	if (group_name.empty())
	{
		std::stringstream message;
		message << "'group_name' can't be empty.";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	if (n_frames <= 0)
	{
		std::stringstream message;
		message << "'n_frames' has to be greater than 0 ('n_frames' = " << n_frames << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	this->cols_groups.push_back(std::make_pair(std::make_pair(group_name, group_description),
	                            std::vector<Reporter::title_t>()));
}

Reporter_probe
::Reporter_probe(const std::string &group_name, const int n_frames)
: Reporter_probe(group_name, "", n_frames)
{
}

int Reporter_probe
::col_size(int col)
{
	return head[col] >= tail[col] ? head[col]-tail[col] : head[col]-tail[col]+this->buffer[col].size();
}

void Reporter_probe
::probe(const std::string &name, const void *data, const int frame_id)
{
	const int col = this->name_to_col[name];
	     if (this->datatypes[col] == typeid(double )) this->push<double >(col, ((double* )data)[0]);
	else if (this->datatypes[col] == typeid(float  )) this->push<float  >(col, ((float*  )data)[0]);
	else if (this->datatypes[col] == typeid(int64_t)) this->push<int64_t>(col, ((int64_t*)data)[0]);
	else if (this->datatypes[col] == typeid(int32_t)) this->push<int32_t>(col, ((int32_t*)data)[0]);
	else if (this->datatypes[col] == typeid(int16_t)) this->push<int16_t>(col, ((int16_t*)data)[0]);
	else if (this->datatypes[col] == typeid(int8_t )) this->push<int8_t >(col, ((int8_t* )data)[0]);
	else
	{
		std::stringstream message;
		message << "Unsupported type.";
		throw tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
	}
}

Reporter::report_t Reporter_probe
::report(bool final)
{
	if (this->cols_groups[0].second.size() == 0)
	{
		std::stringstream message;
		message << "'cols_groups[0].second.size()' has to be greater than 0 ('cols_groups[0].second.size()' = "
		        << this->cols_groups[0].second.size() << ").";
		throw tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (final)
		return this->final_report;

	Reporter::report_t the_report(this->cols_groups.size());
	auto& probe_report = the_report[0];

	bool can_pull = false;
	for (auto f = 0; f < this->n_frames; f++)
	{
		for (size_t col = 0; col < this->buffer.size(); col++)
		{
			std::stringstream stream, temp_stream;
			temp_stream.flags(this->stream_flags[col]);
			if (this->datatypes[col] == typeid(double))
			{
				const double val = this->pull<double>(col, can_pull);
				const std::string s = (val >= 0) ? " " : "";
				if (can_pull) temp_stream << std::setprecision(this->precisions[col]) << s << val;
			}
			else if (this->datatypes[col] == typeid(float))
			{
				const float val = this->pull<float>(col, can_pull);
				const std::string s = (val >= 0) ? " " : "";
				if (can_pull) temp_stream << std::setprecision(this->precisions[col]) << s << val;
			}
			else if (this->datatypes[col] == typeid(int64_t))
			{
				const int64_t val = this->pull<int64_t>(col, can_pull);
				const std::string s = (val >= 0) ? " " : "";
				if (can_pull) temp_stream << std::setprecision(this->precisions[col]) << s << val;
			}
			else if (this->datatypes[col] == typeid(int32_t))
			{
				const int32_t val = this->pull<int32_t>(col, can_pull);
				const std::string s = (val >= 0) ? " " : "";
				if (can_pull) temp_stream << std::setprecision(this->precisions[col]) << s << val;
			}
			else if (this->datatypes[col] == typeid(int16_t))
			{
				const int16_t val = this->pull<int16_t>(col, can_pull);
				const std::string s = (val >= 0) ? " " : "";
				if (can_pull) temp_stream << std::setprecision(this->precisions[col]) << s << val;
			}
			else if (this->datatypes[col] == typeid(int8_t))
			{
				const int8_t val = this->pull<int8_t>(col, can_pull);
				const std::string s = (val >= 0) ? " " : "";
				if (can_pull) temp_stream << std::setprecision(this->precisions[col]) << s << val;
			}
			else
			{
				std::stringstream message;
				message << "Unsupported type.";
				throw tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
			}

			if (!can_pull)
				temp_stream << std::setprecision(this->precisions[col]) << std::scientific << " -";

			stream << std::setprecision(this->precisions[col] +1) << temp_stream.str();
			probe_report.push_back(stream.str());
		}
	}

	this->final_report = the_report;
	return the_report;
}

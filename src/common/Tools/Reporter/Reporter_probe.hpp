/*!
 * \file
 * \brief Class tools::Reporter_probe.
 */
#ifndef REPORTER_PROBE_HPP_
#define REPORTER_PROBE_HPP_

#include <iostream>
#include <aff3ct.hpp>

namespace aff3ct
{
namespace module
{
	template <typename T>
	class Probe;
}
namespace tools
{

class Reporter_probe : public Reporter
{
protected:
	std::vector<int> head;
	std::vector<int> tail;
	std::vector<std::vector<int8_t>> buffer;
	std::vector<std::type_index> datatypes;
	std::vector<std::ios_base::fmtflags> stream_flags;
	std::vector<size_t> precisions;
	std::map<std::string, int> name_to_col;
	const int n_frames;
	Reporter::report_t final_report;

public:
	Reporter_probe(const std::string &group_name,
	               const std::string &group_description,
	               const int n_frames = 1);

	explicit Reporter_probe(const std::string &group_name,
	                        const int n_frames = 1);

	virtual ~Reporter_probe() = default;

	virtual report_t report(bool final = false);

	template <typename T>
	module::Probe<T>* create_probe(const std::string &name,
	                               const std::string &unit,
	                               const std::ios_base::fmtflags ff,
	                               const size_t precision = 3);

	template <typename T>
	module::Probe<T>* create_probe(const std::string &name,
	                               const std::string &unit,
	                               const size_t precision = 3);

	virtual void probe(const std::string &name, const void *data, const int frame_id);

protected:
	int col_size(int col);

	template <typename T>
	void push(const int col, const T &elt);

	template <typename T>
	T pull(const int col, bool &can_pull);
};
}
}

#include "Reporter_probe.hxx"

#endif /* REPORTER_PROBE_HPP_ */

/*!
 * \file
 * \brief Filters a signal.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef PROBE_HXX_
#define PROBE_HXX_

#include <string>
#include <memory>
#include <stdexcept>
#include <cmath>
#include <sstream>

#include "Tools/Exception/exception.hpp"

namespace aff3ct
{
namespace module
{

template <typename R>
Probe<R>::
Probe(const int N, std::string col_id, Reporter_buffered<R>& reporter, const int n_frames)
: Module(n_frames), N(N), col_id(col_id), reporter(reporter)
{
	const std::string name = "Probe";
	this->set_name(name);
	this->set_short_name(name);

	if (N <= 0)
	{
		std::stringstream message;
		message << "'N' has to be greater than 0 ('N' = " << N << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	this->init_processes();
}

template <typename R>
void Probe<R>::
init_processes()
{
	auto &p1 = this->create_task("probe");
	auto p1s_X_N = this->template create_socket_in <R>(p1, "X_N", this->N );
	this->create_codelet(p1, [p1s_X_N](Module &m, Task &t) -> int
	{
		static_cast<Probe<R>&>(m).probe(static_cast<R*>(t[p1s_X_N].get_dataptr()));

		return 0;
	});
}

template <typename R>
template <class AR>
void Probe<R>::
probe(std::vector<R,AR>& X_N, const int frame_id)
{
	if (this->N * this->n_frames != (int)X_N.size())
	{
		std::stringstream message;
		message << "'X_N.size()' has to be equal to 'N' * 'n_frames' ('X_N.size()' = " << X_N.size()
		        << ", 'N' = " << this->N << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	this->probe(X_N.data(), frame_id);
}

template <typename R>
void Probe<R>::
probe(R *X_N, const int frame_id)
{
	this->reporter.probe(this->col_id);
}


}
}

#endif
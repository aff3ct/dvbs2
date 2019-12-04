/*!
 * \file
 * \brief Send data into outer world.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef SINK_HXX_
#define SINK_HXX_

#include <sstream>
#include <complex>

#include "Tools/Exception/exception.hpp"

#include "Module/Sink/Sink.hpp"

namespace aff3ct
{
namespace module
{

template <typename B>
Sink<B>::
Sink(const int N, const int n_frames)
: Module(n_frames), N(N)
{
	const std::string name = "Sink";
	this->set_name(name);
	this->set_short_name(name);

	if (N <= 0)
	{
		std::stringstream message;
		message << "'N' has to be greater than 0 ('N' = " << N << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	auto &p1 = this->create_task("send");
	auto p1s_X_N1 = this->template create_socket_in <B>(p1, "X_N1", N);
	this->create_codelet(p1, [p1s_X_N1](Module& m, Task& t) -> int
	{
		static_cast<Sink<B>&>(m).send(static_cast<B*>(t[p1s_X_N1].get_dataptr()));

		return 0;
	});
}

template <typename B>
template <class A>
void Sink<B>::
send(const std::vector<B,A>& X_N1, const int frame_id)
{
	this->send(X_N1.data(), frame_id);
}

template <typename B>
void Sink<B>::
send(const B *X_N1, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_send(X_N1 + f * this->N, f);
}

}
}

#endif

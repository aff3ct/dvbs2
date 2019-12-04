/*!
 * \file
 * \brief Sens or receive samples to or from a Radio
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef RADIO_HXX_
#define RADIO_HXX_

#include <sstream>
#include <complex>

#include "Tools/Exception/exception.hpp"

#include "Module/Radio/Radio.hpp"

namespace aff3ct
{
namespace module
{

template <typename R>
Radio<R>::
Radio(const int N, const int n_frames)
: Module(n_frames), N(N)
{
	const std::string name = "Radio";
	this->set_name(name);
	this->set_short_name(name);

	if (N <= 0)
	{
		std::stringstream message;
		message << "'N' has to be greater than 0 ('N' = " << N << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	auto &p1 = this->create_task("send");
	auto p1s_X_N1 = this->template create_socket_in <R>(p1, "X_N1", 2 * N);
	this->create_codelet(p1, [p1s_X_N1](Module &m, Task &t) -> int
	{
		static_cast<Radio<R>&>(m).send(static_cast<R*>(t[p1s_X_N1].get_dataptr()));

		return 0;
	});

	auto &p2 = this->create_task("receive");
	auto p2s_Y_N1 = this->template create_socket_out<R>(p2, "Y_N1", 2 * N);
	this->create_codelet(p2, [p2s_Y_N1](Module &m, Task &t) -> int
	{
		static_cast<Radio<R>&>(m).receive(static_cast<R*>(t[p2s_Y_N1].get_dataptr()));

		return 0;
	});
}

template <typename R>
template <class A>
void Radio<R>::
send(const std::vector<R,A>& X_N1, const int frame_id)
{
	this->send(X_N1.data(), frame_id);
}

template <typename R>
void Radio<R>::
send(const R *X_N1, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_send(X_N1 + f * this->N * 2, f);
}

template <typename R>
template <class A>
void Radio<R>::
receive(std::vector<R,A>& Y_N1, const int frame_id)
{
	this->receive(Y_N1.data(), frame_id);
}

template <typename R>
void Radio<R>::
receive(R *Y_N1, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_receive(Y_N1 + f * this->N * 2, f);
}

}
}

#endif

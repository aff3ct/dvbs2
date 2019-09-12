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

#include "Radio.hpp"

namespace aff3ct
{
namespace module
{

template <typename D>
Radio<D>::
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
	auto &p1s_X_N1 = this->template create_socket_in <D>(p1, "X_N1", 2 * N * this->n_frames);
	this->create_codelet(p1, [this, &p1s_X_N1]() -> int
	{
		this->send(static_cast<D*>(p1s_X_N1.get_dataptr()));

		return 0;
	});

	auto &p2 = this->create_task("receive");
	auto &p2s_Y_N1 = this->template create_socket_in <D>(p2, "Y_N1", 2 * N  * this->n_frames);
	this->create_codelet(p2, [this, &p2s_Y_N1]() -> int
	{
		this->receive(static_cast<D*>(p2s_Y_N1.get_dataptr()));

		return 0;
	});
}

template <typename D>
template <class A>
void Radio<D>::
send(std::vector<D,A>& X_N1, const int frame_id)
{
	this->send(X_N1.data(), frame_id);
}

template <typename D>
void Radio<D>::
send(D *X_N1, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_send(X_N1 + f * this->XFEC_FRAME_SIZE, f);
}

template <typename D>
template <class A>
void Radio<D>::
receive(std::vector<D,A>& Y_N1, const int frame_id)
{
	this->receive(Y_N1.data(), frame_id);
}

template <typename D>
void Radio<D>::
receive(D *Y_N1, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_receive(Y_N1 + f * this->PL_FRAME_SIZE, f);
}


}
}

#endif

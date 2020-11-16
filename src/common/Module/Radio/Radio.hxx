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
Radio<R>
::Radio(const int N, const int n_frames)
: Module(), N(N),
  ovf_flags(n_frames, 0),
  seq_flags(n_frames, 0),
  clt_flags(n_frames, 0),
  tim_flags(n_frames, 0)
{
	const std::string name = "Radio";
	this->set_name(name);
	this->set_short_name(name);
	this->set_n_frames(n_frames);
	this->set_single_wave(true);

	if (N <= 0)
	{
		std::stringstream message;
		message << "'N' has to be greater than 0 ('N' = " << N << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	auto &p1 = this->create_task("send");
	auto p1s_X_N1 = this->template create_socket_in <R>(p1, "X_N1", 2 * N);
	this->create_codelet(p1, [p1s_X_N1](Module &m, Task &t, const size_t frame_id) -> int
	{
		static_cast<Radio<R>&>(m).send(static_cast<R*>(t[p1s_X_N1].get_dataptr()));
		return 0;
	});

	auto &p2 = this->create_task("receive");
	auto p2s_OVF  = this->template create_socket_out<int32_t>(p2, "OVF" , 1    );
	auto p2s_SEQ  = this->template create_socket_out<int32_t>(p2, "SEQ" , 1    );
	auto p2s_CLT  = this->template create_socket_out<int32_t>(p2, "CLT" , 1    );
	auto p2s_TIM  = this->template create_socket_out<int32_t>(p2, "TIM" , 1    );
	auto p2s_Y_N1 = this->template create_socket_out<R      >(p2, "Y_N1", 2 * N);
	this->create_codelet(p2, [p2s_OVF, p2s_SEQ, p2s_CLT, p2s_TIM, p2s_Y_N1](Module &m, Task &t, const size_t frame_id) -> int
	{
		static_cast<Radio<R>&>(m).receive(static_cast<int32_t*>(t[p2s_OVF ].get_dataptr()),
		                                  static_cast<int32_t*>(t[p2s_SEQ ].get_dataptr()),
		                                  static_cast<int32_t*>(t[p2s_CLT ].get_dataptr()),
		                                  static_cast<int32_t*>(t[p2s_TIM ].get_dataptr()),
		                                  static_cast<R*      >(t[p2s_Y_N1].get_dataptr()));

		return 0;
	});
}

template <typename R>
template <class A>
void Radio<R>
::send(const std::vector<R,A>& X_N1, const int frame_id)
{
	this->send(X_N1.data(), frame_id);
}

template <typename R>
void Radio<R>
::send(const R *X_N1, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_send(X_N1 + f * this->N * 2, f);
}

template <typename R>
template <class A>
void Radio<R>
::receive(std::vector<int32_t>& OVF,
          std::vector<int32_t>& SEQ,
          std::vector<int32_t>& CLT,
          std::vector<int32_t>& TIM,
          std::vector<R,A>& Y_N1,
          const int frame_id)
{
	this->receive(OVF.data(), SEQ.data(), CLT.data(), TIM.data(), Y_N1.data(), frame_id);
}

template <typename R>
void Radio<R>
::receive(int32_t *OVF, int32_t *SEQ, int32_t *CLT, int32_t *TIM, R *Y_N1, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_receive(Y_N1 + f * this->N * 2, f);

	for (auto f = f_start; f < f_stop; f++)
	{
		OVF[f] = this->ovf_flags[f];
		this->ovf_flags[f] = 0;
		SEQ[f] = this->seq_flags[f];
		this->seq_flags[f] = 0;
		CLT[f] = this->clt_flags[f];
		this->clt_flags[f] = 0;
		TIM[f] = this->tim_flags[f];
		this->tim_flags[f] = 0;
	}
}

}
}

#endif

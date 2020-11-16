/*!
 * \file
 * \brief Filters a signal.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef SYNCHRONIZER_HXX_
#define SYNCHRONIZER_HXX_

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
Synchronizer<R>::
Synchronizer(const int N_in, const int N_out, const int n_frames)
: Module(), N_in(N_in), N_out(N_out)
{
	const std::string name = "Synchronizer";
	this->set_name(name);
	this->set_short_name(name);
	this->set_n_frames(n_frames);
	this->set_single_wave(true);

	if (N_in <= 0)
	{
		std::stringstream message;
		message << "'N_in' has to be greater than 0 ('N_in' = " << N_in << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	if (N_out <= 0)
	{
		std::stringstream message;
		message << "'N_out' has to be greater than 0 ('N_out' = " << N_out << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	this->init_processes();
}

template <typename R>
void Synchronizer<R>::
init_processes()
{
	auto &p1 = this->create_task("synchronize");
	auto p1s_X_N1 = this->template create_socket_in <R>(p1, "X_N1", this->N_in );
	auto p1s_Y_N2 = this->template create_socket_out<R>(p1, "Y_N2", this->N_out);
	this->create_codelet(p1, [p1s_X_N1, p1s_Y_N2](Module &m, Task &t, const size_t frame_id) -> int
	{
		static_cast<Synchronizer<R>&>(m).synchronize(static_cast<R*>(t[p1s_X_N1].get_dataptr()),
		                                             static_cast<R*>(t[p1s_Y_N2].get_dataptr()));

		return 0;
	});
}

template <typename R>
int Synchronizer<R>::
get_N_in() const
{
	return this->N_in;
}

template <typename R>
int Synchronizer<R>::
get_N_out() const
{
	return this->N_out;
}

template <typename R>
template <class AR>
void Synchronizer<R>::
synchronize(const std::vector<R,AR>& X_N1, std::vector<R,AR>& Y_N2, const int frame_id)
{
	if (this->N_in * this->n_frames != (int)X_N1.size())
	{
		std::stringstream message;
		message << "'X_N1.size()' has to be equal to 'N' * 'n_frames' ('X_N1.size()' = " << X_N1.size()
		        << ", 'N' = " << this->N_in << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (this->N_out * this->n_frames != (int)Y_N2.size())
	{
		std::stringstream message;
		message << "'Y_N2.size()' has to be equal to 'N_fil' * 'n_frames' ('Y_N2.size()' = " << Y_N2.size()
		        << ", 'N_fil' = " << this->N_out << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	this->synchronize(X_N1.data(), Y_N2.data(), frame_id);
}

template <typename R>
void Synchronizer<R>::
synchronize(const R *X_N1, R *Y_N2, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_synchronize(X_N1 + f * this->N_in,
		                   Y_N2 + f * this->N_out,
		                   f);
}

template <typename R>
void Synchronizer<R>::
_synchronize(const R *X_N1, R *Y_N2, const int frame_id)
{
	throw tools::unimplemented_error(__FILE__, __LINE__, __func__);
}

}
}

#endif

/*!
 * \file
 * \brief Filters a signal.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef MULTIPLIER_HXX_
#define MULTIPLIER_HXX_

#include <stdexcept>
#include <cmath>
#include <sstream>

#include "Tools/Exception/exception.hpp"

#include "Module/Multiplier/Multiplier.hpp"

namespace aff3ct
{
namespace module
{

template <typename R>
Multiplier<R>::
Multiplier(const int N, const int n_frames)
: Module(), N(N)
{
	const std::string name = "Multiplier";
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

	this->init_processes();
}

template <typename R>
void Multiplier<R>::
init_processes()
{
	auto &p1 = this->create_task("imultiply");
	auto p1s_X_N = this->template create_socket_in <R>(p1, "X_N", this->N);
	auto p1s_Z_N = this->template create_socket_out<R>(p1, "Z_N", this->N);
	this->create_codelet(p1, [p1s_X_N, p1s_Z_N](Module &m, Task &t, const size_t frame_id) -> int
	{
		static_cast<Multiplier<R>&>(m).imultiply(static_cast<R*>(t[p1s_X_N].get_dataptr()),
		                                         static_cast<R*>(t[p1s_Z_N].get_dataptr()));

		return 0;
	});

	auto &p2 = this->create_task("multiply");
	auto p2s_X_N = this->template create_socket_in <R>(p2, "X_N", this->N);
	auto p2s_Y_N = this->template create_socket_in <R>(p2, "Y_N", this->N);
	auto p2s_Z_N = this->template create_socket_out<R>(p2, "Z_N", this->N);
	this->create_codelet(p2, [p2s_X_N, p2s_Y_N, p2s_Z_N](Module &m, Task &t, const size_t frame_id) -> int
	{
		static_cast<Multiplier<R>&>(m).multiply(static_cast<R*>(t[p2s_X_N].get_dataptr()),
		                                        static_cast<R*>(t[p2s_Y_N].get_dataptr()),
		                                        static_cast<R*>(t[p2s_Z_N].get_dataptr()));

		return 0;
	});
}

template <typename R>
int Multiplier<R>::
get_N() const
{
	return this->N;
}

template <typename R>
template <class AR>
void Multiplier<R>::
imultiply(const std::vector<R,AR>& X_N, std::vector<R,AR>& Z_N, const int frame_id)
{
	if (this->N * this->n_frames != (int)X_N.size())
	{
		std::stringstream message;
		message << "'X_N.size()' has to be equal to 'N' * 'n_frames' ('X_N.size()' = " << X_N.size()
		        << ", 'N' = " << this->N << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (this->N * this->n_frames != (int)Z_N.size())
	{
		std::stringstream message;
		message << "'Z_N.size()' has to be equal to 'N' * 'n_frames' ('Z_N.size()' = " << Z_N.size()
		        << ", 'N' = " << this->N << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	this->imultiply(X_N.data(), Z_N.data(), frame_id);
}

template <typename R>
void Multiplier<R>::
imultiply(const R *X_N, R *Z_N, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_imultiply(X_N + f * this->N,
		                 Z_N + f * this->N,
		                 f);
}

template <typename R>
template <class AR>
void Multiplier<R>::
multiply(const std::vector<R,AR>& X_N, const std::vector<R,AR>& Y_N, std::vector<R,AR>& Z_N, const int frame_id)
{
	if (this->N * this->n_frames != (int)X_N.size())
	{
		std::stringstream message;
		message << "'X_N.size()' has to be equal to 'N' * 'n_frames' ('X_N.size()' = " << X_N.size()
		        << ", 'N' = " << this->N << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (this->N * this->n_frames != (int)Y_N.size())
	{
		std::stringstream message;
		message << "'Y_N.size()' has to be equal to 'N' * 'n_frames' ('Y_N.size()' = " << Y_N.size()
		        << ", 'N' = " << this->N << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (this->N * this->n_frames != (int)Z_N.size())
	{
		std::stringstream message;
		message << "'Z_N.size()' has to be equal to 'N' * 'n_frames' ('Z_N.size()' = " << Z_N.size()
		        << ", 'N' = " << this->N << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	this->multiply(X_N.data(), Y_N.data(), Z_N.data(), frame_id);
}

template <typename R>
void Multiplier<R>::
multiply(const R *X_N, const R *Y_N, R *Z_N, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_multiply(X_N + f * this->N,
		                Y_N + f * this->N,
		                Z_N + f * this->N,
		                f);
}

template <typename R>
void Multiplier<R>::
_imultiply(const R *X_N, R *Z_N, const int frame_id)
{
	throw tools::unimplemented_error(__FILE__, __LINE__, __func__);
}

template <typename R>
void Multiplier<R>::
_multiply(const R *X_N, const R *Y_N, R *Z_N, const int frame_id)
{
	throw tools::unimplemented_error(__FILE__, __LINE__, __func__);
}
}
}

#endif

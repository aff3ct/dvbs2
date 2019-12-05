/*!
 * \file
 * \brief Generates a message.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef ESTIMATOR_HXX_
#define ESTIMATOR_HXX_

#include <sstream>
#include <complex>

#include "Tools/Exception/exception.hpp"

#include "Module/Estimator/Estimator.hpp"

namespace aff3ct
{
namespace module
{

template <typename R>
Estimator<R>::
Estimator(const int N, const int n_frames)
: Module(n_frames), N(N), noise(nullptr)
{
	const std::string name = "Estimator";
	this->set_name(name);
	this->set_short_name(name);

	if (N <= 0)
	{
		std::stringstream message;
		message << "'N' has to be greater than 0 ('N' = " << N << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	auto &p1 = this->create_task("estimate");
	auto p1s_X_N = this->template create_socket_out<R>(p1, "X_N", this->N);
	auto p1s_H_N = this->template create_socket_out<R>(p1, "H_N", this->N);
	this->create_codelet(p1, [p1s_X_N, p1s_H_N](Module& m, Task& t) -> int
	{
		static_cast<Estimator<R>&>(m).estimate(static_cast<R*>(t[p1s_X_N].get_dataptr()),
		                                       static_cast<R*>(t[p1s_H_N].get_dataptr()));

		return 0;
	});
}

template <typename R>
int Estimator<R>::
get_N() const
{
	return N;
}

template <typename R>
template <class A>
void Estimator<R>::
estimate(std::vector<R,A>& X_N, std::vector<R,A>& H_N, const int frame_id)
{
	if (this->N * this->n_frames != (int)X_N.size())
	{
		std::stringstream message;
		message << "'X_N.size()' has to be equal to 'N' * 'n_frames' ('X_N.size()' = " << X_N.size()
		        << ", 'N' = " << this->N << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}
	if (this->N * this->n_frames != (int)H_N.size())
	{
		std::stringstream message;
		message << "'H_N.size()' has to be equal to 'N' * 'n_frames' ('H_N.size()' = " << H_N.size()
		        << ", 'N' = " << this->N << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	this->estimate(X_N.data(), H_N.data(), frame_id);
}

template <typename R>
void Estimator<R>::
estimate(R *X_N, R *H_N, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_estimate(X_N + f * this->N, H_N + f * this->N, f);
}

template <typename R>
void Estimator<R>
::set_noise(const tools::Noise<>& noise)
{
	this->noise = &noise;
	this->check_noise();
}

template<typename R>
const tools::Noise<>& Estimator<R>
::get_noise() const
{
	this->check_noise();
	return *this->noise;
}


template<typename R>
void Estimator<R>
::check_noise()
{
	if (this->noise == nullptr)
	{
		std::stringstream message;
		message << "'noise' should not be nullptr.";
		throw tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
	}
}


template <typename R>
void Estimator<R>::
_estimate(R *X_N, R *H_N, const int frame_id)
{
	throw tools::unimplemented_error(__FILE__, __LINE__, __func__);
}

}
}

#endif

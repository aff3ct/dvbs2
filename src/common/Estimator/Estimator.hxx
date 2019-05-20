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

#include "Estimator.hpp"

namespace aff3ct
{
namespace module
{

template <typename R>
Estimator<R>::
Estimator(const int N, const int n_frames)
: Module(n_frames), N(N)
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
	auto &p1s_X_N = this->template create_socket_out<R>(p1, "Y_N", this->N * this->n_frames);
	auto &p1s_H_N = this->template create_socket_out<R>(p1, "H_N", this->N * this->n_frames);
	this->create_codelet(p1, [this, &p1s_X_N, &p1s_H_N]() -> int
	{
		this->estimate(static_cast<R*>(p1s_X_N.get_dataptr()),
		               static_cast<R*>(p1s_H_N.get_dataptr()));

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
R Estimator<R>::
get_sigma_n2()
{
	return sigma_n2;
}

template <typename R>
void Estimator<R>::
_estimate(R *X_N, R *H_N, const int frame_id)
{	
	float moment2 = 0, moment4 = 0;
	float pow_tot, pow_sig_util;

	for (int i = 0; i < this->N / 2; i++)
	{
		float tmp = X_N[2 * i]*X_N[2 * i] + X_N[2 * i + 1]*X_N[2 * i + 1];
		moment2 += tmp;
		moment4 += tmp*tmp;
	}
	moment2 /= this->N / 2;
	moment4 /= this->N / 2;

	float Se      = std::sqrt( std::abs(2 * moment2 * moment2 - moment4 ) );
	float Ne      = std::abs( moment2 - Se );
	float SNR_est = 10 * std::log10(Se / Ne);

	SNR_est = 15.8;

	std::cout << "(WW) SNR Estimation is manually set." << std::endl;
	pow_tot = moment2;

	pow_sig_util = pow_tot / (1+(std::pow(10, (-1 * SNR_est/10))));

	this->sigma_n2 = pow_tot - pow_sig_util;

	float H = std::sqrt(pow_sig_util);

	for (int i = 0; i < this->N / 2; i++)
	{
		H_N[2*i] = H;
		H_N[2*i+1] = 0;
	}
}

}
}

#endif

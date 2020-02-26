/*!
 * \file
 * \brief Generates a message.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef ESTIMATOR_DVBS2O_HXX
#define ESTIMATOR_DVBS2O_HXX

#include <sstream>
#include <complex>

#include "Tools/Exception/exception.hpp"

#include "Module/Estimator/Estimator_DVBS2O.hpp"

namespace aff3ct
{
namespace module
{

template <typename R>
Estimator_DVBS2O<R>::
Estimator_DVBS2O(const int N, const float code_rate, const int bps, const int n_frames)
: Estimator<R>(N, n_frames), bps(bps), code_rate(code_rate)
{
}

template <typename R>
Estimator_DVBS2O<R>* Estimator_DVBS2O<R>
::clone() const
{
	auto m = new Estimator_DVBS2O(*this);
	m->deep_copy(*this);
	return m;
}

template<typename R>
void Estimator_DVBS2O<R>
::check_noise()
{
	Estimator<R>::check_noise();
	this->noise->is_of_type_throw(tools::Noise_type::SIGMA);
}


template <typename R>
void Estimator_DVBS2O<R>::
_estimate(const R *X_N, R *H_N, const int frame_id)
{
	this->check_noise();

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
	float esn0_estimated = 10 * std::log10(Se / Ne);

	pow_tot = moment2;

	pow_sig_util = pow_tot / (1+(std::pow(10, (-1 * esn0_estimated/10))));

	const auto sigma_estimated = tools::esn0_to_sigma(esn0_estimated);
	const auto ebn0_estimated  = tools::esn0_to_ebn0(esn0_estimated, code_rate, bps);

	tools::Sigma<R> * sigma = dynamic_cast<tools::Sigma<R>*>(this->noise);
	sigma->set_values(sigma_estimated, ebn0_estimated, esn0_estimated);

	float H = std::sqrt(pow_sig_util);

	for (int i = 0; i < this->N / 2; i++)
	{
		H_N[2*i] = H;
		H_N[2*i+1] = 0;
	}

	this->ebn0_estimated = ebn0_estimated;
	this->esn0_estimated = esn0_estimated;
}

template <typename R>
void Estimator_DVBS2O<R>::
_rescale(const R *X_N, R *H_N, R *Y_N, const int frame_id)
{
	float moment2 = 0, moment4 = 0;

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
	float esn0_estimated = 10 * std::log10(Se / Ne);

	const auto sigma_estimated = tools::esn0_to_sigma(esn0_estimated);
	const auto ebn0_estimated  = tools::esn0_to_ebn0(esn0_estimated, code_rate, bps);

	// hack to have the SNR displayed in the terminal
	tools::Sigma<R> * sigma = dynamic_cast<tools::Sigma<R>*>(this->noise);
	sigma->set_values(sigma_estimated, ebn0_estimated, esn0_estimated);

	float H_ = std::sqrt(2*esn0_estimated);
	for (int i = 0; i < this->N / 2; i++)
	{
		Y_N[2*i  ] = X_N[2*i  ] / sigma_estimated;
		Y_N[2*i+1] = X_N[2*i+1] / sigma_estimated;

		H_N[2*i  ] = H_;
		H_N[2*i+1] = 0;
	}

	this->ebn0_estimated = ebn0_estimated;
	this->esn0_estimated = esn0_estimated;
}
}
}
#endif

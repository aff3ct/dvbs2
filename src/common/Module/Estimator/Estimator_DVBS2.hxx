#include <sstream>
#include <complex>

#include "Tools/Exception/exception.hpp"

#include "Module/Estimator/Estimator_DVBS2.hpp"

namespace aff3ct
{
namespace module
{

template <typename R>
Estimator_DVBS2<R>::
Estimator_DVBS2(const int N, const float code_rate, const int bps, const int n_frames)
: Estimator<R>(N, n_frames), bps(bps), code_rate(code_rate)
{
}

template <typename R>
Estimator_DVBS2<R>* Estimator_DVBS2<R>
::clone() const
{
	auto m = new Estimator_DVBS2(*this);
	m->deep_copy(*this);
	return m;
}

template <typename R>
void Estimator_DVBS2<R>::
_estimate(const R *X_N, const int frame_id)
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
	// saturate 'esn0_estimated' to 100 dB
	esn0_estimated = std::isinf(esn0_estimated) ? 100. : esn0_estimated;

	const auto sigma_estimated = tools::esn0_to_sigma(esn0_estimated);
	const auto ebn0_estimated  = tools::esn0_to_ebn0(esn0_estimated, code_rate, bps);

	this->sigma_estimated = sigma_estimated;
	this->ebn0_estimated = ebn0_estimated;
	this->esn0_estimated = esn0_estimated;
}

template <typename R>
void Estimator_DVBS2<R>::
_rescale(const R *X_N, R *Y_N, const int frame_id)
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
	// saturate 'esn0_estimated' to 100 dB
	esn0_estimated = std::isinf(esn0_estimated) ? 100. : esn0_estimated;

	const auto sigma_estimated = tools::esn0_to_sigma(esn0_estimated);
	const auto ebn0_estimated  = tools::esn0_to_ebn0(esn0_estimated, code_rate, bps);

	for (int i = 0; i < this->N / 2; i++)
	{
		Y_N[2*i  ] = X_N[2*i  ] / sigma_estimated;
		Y_N[2*i+1] = X_N[2*i+1] / sigma_estimated;
	}

	this->sigma_estimated = sigma_estimated;
	this->ebn0_estimated  = ebn0_estimated;
	this->esn0_estimated  = esn0_estimated;
}
}
}

#include <cassert>
#include <iostream>

#include "Synchronizer_freq_phase_DVBS2_aib.hpp"

// _USE_MATH_DEFINES does not seem to work on MSVC...
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

using namespace aff3ct::module;

template <typename R>
Synchronizer_freq_phase_DVBS2_aib<R>
::Synchronizer_freq_phase_DVBS2_aib(const int N, const int n_frames)
: Synchronizer_freq_fine<R>(N, n_frames), pilot_start()
{
	int idx = 1530;
	while (idx < N/2)
	{
		pilot_start.push_back(idx);
		idx += 1476;
	}
}

template <typename R>
Synchronizer_freq_phase_DVBS2_aib<R>
::~Synchronizer_freq_phase_DVBS2_aib()
{}

template <typename R>
Synchronizer_freq_phase_DVBS2_aib<R>* Synchronizer_freq_phase_DVBS2_aib<R>
::clone() const
{
	auto m = new Synchronizer_freq_phase_DVBS2_aib(*this);
	m->deep_copy(*this);
	return m;
}

template <typename R>
void Synchronizer_freq_phase_DVBS2_aib<R>
::_synchronize(const R *X_N1, R *Y_N2, const int frame_id)
{
	int P  = pilot_start.size();
	int Lp = 36;

	R inv_2PI = 1.0f/(2*M_PI);

	std::vector<R> phase_est(P, (R)0);
	for (int p = 0; p < P; p++)
	{
		std::vector<R> sum_symb_conj_ref(2, (R)0);
		// Phase estimation for every pilot location
		for (int i = 0 ; i < Lp ; i++)
		{
			sum_symb_conj_ref[0] += X_N1[2*this->pilot_start[p] + 2*i    ]
			                      + X_N1[2*this->pilot_start[p] + 2*i + 1];

			sum_symb_conj_ref[1] += X_N1[2*this->pilot_start[p] + 2*i + 1]
			                      - X_N1[2*this->pilot_start[p] + 2*i    ];
		}
		phase_est[p] = std::atan2(sum_symb_conj_ref[1], sum_symb_conj_ref[0]);
		phase_est[p] = phase_est[p] < 0 ? phase_est[p] + 2*M_PI : phase_est[p];
	}

	std::vector<R> y(P,0.0f);
	std::vector<R> t(P,0.0f);

	y[0] = inv_2PI*phase_est[0];
	t[0] = this->pilot_start[0] + (R)(Lp/2);

	R acc = (R)0;

	for (int p = 1; p<P; p++)
	{
		R diff_angle = phase_est[p] - phase_est[p-1];
		R acc_elt = diff_angle > 0 ? std::floor(diff_angle*inv_2PI + (R)0.5) :  std::ceil(diff_angle*inv_2PI - (R)0.5);
		acc_elt   = std::fabs(diff_angle) > M_PI ? acc_elt : 0.0f;
		acc += acc_elt;
		y[p] = inv_2PI*phase_est[p] - acc;
		t[p] = this->pilot_start[p] + (R)(Lp/2);
	}

	R sum_t   = (R)0;
	R sum_y   = (R)0;
	R sum_ty  = (R)0;
	R sum_tt  = (R)0;

	for (int p = 0; p < P; p++)
	{
		sum_t  += t[p];
		sum_y  += y[p];
		sum_ty += t[p]*y[p];
		sum_tt += t[p]*t[p];
	}

	this->estimated_freq  = (P * sum_ty - sum_t * sum_y) / (P * sum_tt - sum_t * sum_t);
	this->estimated_phase = (sum_y - this->estimated_freq * sum_t) / P;

	for (int n = 0 ; n < this->N/2 ; n++)
	{
		R theta = 2 * M_PI *(this->estimated_freq * (R)n + this->estimated_phase);

		Y_N2[2*n    ] = X_N1[2*n    ] * std::cos(theta)
		              + X_N1[2*n + 1] * std::sin(theta);

		Y_N2[2*n + 1] = X_N1[2*n + 1] * std::cos(theta)
		              - X_N1[2*n    ] * std::sin(theta);
	}
}

template <typename R>
void Synchronizer_freq_phase_DVBS2_aib<R>
::_reset()
{
	this->estimated_freq  = (R)0;
	this->estimated_phase = (R)0;
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Synchronizer_freq_phase_DVBS2_aib<float>;
template class aff3ct::module::Synchronizer_freq_phase_DVBS2_aib<double>;
// ==================================================================================== explicit template instantiation

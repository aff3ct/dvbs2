#include <cassert>
#include <iostream>

#include "Module/Synchronizer/Synchronizer_freq/Synchronizer_freq_fine/Synchronizer_Luise_Reggiannini_DVBS2_aib.hpp"

// _USE_MATH_DEFINES does not seem to work on MSVC...
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

using namespace aff3ct::module;

template <typename R>
Synchronizer_Luise_Reggiannini_DVBS2_aib<R>
::Synchronizer_Luise_Reggiannini_DVBS2_aib(const int N, const std::vector<R> pilot_values, const std::vector<int> pilot_start)
: Synchronizer_freq<R>(N), pilot_size(pilot_values.size()), pilot_nbr(pilot_start.size()), pilot_values(pilot_values), pilot_start(pilot_start), R_l(2,(R)0.0)
{
	assert(pilot_size > 0);
	assert(pilot_nbr  > 0);
	assert(N > pilot_start[pilot_nbr-1] + pilot_size - 1);
}

template <typename R>
Synchronizer_Luise_Reggiannini_DVBS2_aib<R>
::~Synchronizer_Luise_Reggiannini_DVBS2_aib()
{}

template <typename R>
void Synchronizer_Luise_Reggiannini_DVBS2_aib<R>
::_synchronize(const R *X_N1, R *Y_N2, const int frame_id)
{
	int P = this->pilot_nbr;
	int Lp = this->pilot_size/2;
	int Lp_2 = Lp/2;
	for  (int p = 0 ; p<P ; p++)
	{
		//std::cout << this->pilot_start[p] << std::endl;
		std::vector<R> z(2*Lp, (R)0.0);
		for (int i = 0 ; i<Lp ; i++)
		{
			z[2*i    ] = X_N1[2*i + 2*this->pilot_start[p]    ] * pilot_values[2*i    ]
			           + X_N1[2*i + 2*this->pilot_start[p] + 1] * pilot_values[2*i + 1];

			z[2*i + 1] = X_N1[2*i + 2*this->pilot_start[p] + 1] * pilot_values[2*i    ]
			           - X_N1[2*i + 2*this->pilot_start[p]    ] * pilot_values[2*i + 1];

		}

		for (int m = 1 ; m < Lp_2 + 1 ; m++)
		{
			std::vector<float  > sum_xcorr_z(2, 0.0f);

			for (int k = m; k < Lp ; k++ )
			{
				sum_xcorr_z[0] += z[2*k    ] * z[2*(k-m)    ]
				                + z[2*k + 1] * z[2*(k-m) + 1];

				sum_xcorr_z[1] += z[2*k + 1] * z[2*(k-m)    ]
				                - z[2*k    ] * z[2*(k-m) + 1];
			}
			this->R_l[0] += sum_xcorr_z[0] / (R)(Lp-m);
			this->R_l[1] += sum_xcorr_z[1] / (R)(Lp-m);
		}
	}
	this->estimated_freq = std::atan2(this->R_l[1], this->R_l[0]);
	this->estimated_freq /= (Lp_2 + 1) * M_PI;

	for (int n = 0 ; n < this->N_in/2 ; n++)
	{
		R theta = 2 * M_PI * this->estimated_freq * (R)n;
		R cos_theta = std::cos(theta);
		R sin_theta = std::sin(theta);

		Y_N2[2*n    ] = X_N1[2*n    ] * cos_theta
		              + X_N1[2*n + 1] * sin_theta;

		Y_N2[2*n + 1] = X_N1[2*n + 1] * cos_theta
		              - X_N1[2*n    ] * sin_theta;
	}
}

template <typename R>
void Synchronizer_Luise_Reggiannini_DVBS2_aib<R>
::_reset()
{
	this->R_l[0]         = (R)0.0;
	this->R_l[1]         = (R)0.0;
	this->estimated_freq = (R)0.0;
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Synchronizer_Luise_Reggiannini_DVBS2_aib<float>;
template class aff3ct::module::Synchronizer_Luise_Reggiannini_DVBS2_aib<double>;
// ==================================================================================== explicit template instantiation

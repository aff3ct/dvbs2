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
::Synchronizer_Luise_Reggiannini_DVBS2_aib(const int N, const int n_frames)
: Synchronizer_freq_fine<R>(N, n_frames), pilot_nbr(0), pilot_start(), R_l(2,(R)0.0)
{
	int idx = 1530;
	while (idx < N/2)
	{
		pilot_start.push_back(idx);
		pilot_nbr++;
		idx += 1476;
	}

	if (mipp::N<R>() > 1)
	{
		for (auto i = 0; i < mipp::N<R>()/2; i++)
			n_vals[2*i +0] = (R)(2. * i);
		for (auto i = 0; i < mipp::N<R>()/2; i++)
			n_vals[2*i +1] = (R)mipp::N<R>() + (R)(2. * i);
	}
	else
		n_vals[0] = 0;

	for (auto i = 0; i < mipp::N<R>(); i++)
		mask[i] = !(i % 2);
}

template <typename R>
Synchronizer_Luise_Reggiannini_DVBS2_aib<R>
::~Synchronizer_Luise_Reggiannini_DVBS2_aib()
{}

template <typename R>
void Synchronizer_Luise_Reggiannini_DVBS2_aib<R>
::_synchronize_old(const R *X_N1, R *Y_N2, const int frame_id)
{
	int P = this->pilot_nbr;
	int Lp = 36/2;
	int Lp_2 = Lp/2;
	for  (int p = 0 ; p<P ; p++)
	{
		//std::cout << this->pilot_start[p] << std::endl;
		std::vector<R> z(2*Lp, (R)0.0);
		for (int i = 0 ; i<Lp ; i++)
		{
			z[2*i    ] = X_N1[2*i + 2*this->pilot_start[p]    ]
			           + X_N1[2*i + 2*this->pilot_start[p] + 1];

			z[2*i + 1] = X_N1[2*i + 2*this->pilot_start[p] + 1]
			           - X_N1[2*i + 2*this->pilot_start[p]    ];

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
			this->R_l[0] += sum_xcorr_z[0] / (R)(2*(Lp-m));
			this->R_l[1] += sum_xcorr_z[1] / (R)(2*(Lp-m));
		}
	}
	this->estimated_freq = std::atan2(this->R_l[1], this->R_l[0]);
	this->estimated_freq /= (Lp_2 + 1) * M_PI;

	for (int n = 0 ; n < this->N/2 ; n++)
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
::_synchronize(const R *X_N1, R *Y_N2, const int frame_id)
{
	int P = this->pilot_nbr;
	int Lp = 36/2;
	int Lp_2 = Lp/2;
	for  (int p = 0 ; p<P ; p++)
	{
		//std::cout << this->pilot_start[p] << std::endl;
		std::vector<R> z(2*Lp, (R)0.0);
		for (int i = 0 ; i<Lp ; i++)
		{
			z[2*i    ] = X_N1[2*i + 2*this->pilot_start[p]    ]
			           + X_N1[2*i + 2*this->pilot_start[p] + 1];

			z[2*i + 1] = X_N1[2*i + 2*this->pilot_start[p] + 1]
			           - X_N1[2*i + 2*this->pilot_start[p]    ];

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
			this->R_l[0] += sum_xcorr_z[0] / (R)(2*(Lp-m));
			this->R_l[1] += sum_xcorr_z[1] / (R)(2*(Lp-m));
		}
	}
	this->estimated_freq = std::atan2(this->R_l[1], this->R_l[0]);
	this->estimated_freq /= (Lp_2 + 1) * M_PI;
	this->estimated_freq *= M_PI;

	mipp::Msk<mipp::N<R>()> reg_msk = this->mask;
	mipp::Reg<R> reg_n = this->n_vals;
	mipp::Reg<R> reg_estimated_freq = this->estimated_freq;
	mipp::Reg<R> reg_cos_theta;
	mipp::Reg<R> reg_sin_theta;

	auto end_vec_loop = (this->N / (2 * mipp::N<R>())) * (2 * mipp::N<R>());
	for (int n = 0; n < end_vec_loop; n += 2 * mipp::N<R>())
	{
		auto reg_theta = reg_estimated_freq * reg_n;

		mipp::sincos(reg_theta, reg_sin_theta, reg_cos_theta);

		mipp::Reg<R> reg_X_N1_1 = &X_N1[n + 0 * mipp::N<R>()];
		mipp::Reg<R> reg_X_N1_2 = &X_N1[n + 1 * mipp::N<R>()];

		auto reg_X_N1_2r = mipp::rrot(reg_X_N1_2);
		auto reg_X_N1_1r = mipp::lrot(reg_X_N1_1);

		auto reg_X_N1_re = mipp::blend(reg_X_N1_1 , reg_X_N1_2r, reg_msk);
		auto reg_X_N1_im = mipp::blend(reg_X_N1_1r, reg_X_N1_2 , reg_msk);

		auto reg_re = (reg_X_N1_re * reg_cos_theta) + (reg_X_N1_im * reg_sin_theta);
		auto reg_im = (reg_X_N1_im * reg_cos_theta) - (reg_X_N1_re * reg_sin_theta);

		auto reg_imr = mipp::rrot(reg_im);
		auto reg_rer = mipp::lrot(reg_re);

		auto reg_Y_N2_1 = mipp::blend(reg_re , reg_imr, reg_msk);
		auto reg_Y_N2_2 = mipp::blend(reg_rer, reg_im , reg_msk);

		reg_Y_N2_1.store(&Y_N2[n + 0 * mipp::N<R>()]);
		reg_Y_N2_2.store(&Y_N2[n + 1 * mipp::N<R>()]);

		reg_n += (R)(2 * mipp::N<R>());
	}

	for (int n = end_vec_loop; n < this->N; n += 2)
	{
		R theta = this->estimated_freq * (R)n;
		R cos_theta = std::cos(theta);
		R sin_theta = std::sin(theta);

		Y_N2[n +0] = X_N1[n +0] * cos_theta + X_N1[n +1] * sin_theta;
		Y_N2[n +1] = X_N1[n +1] * cos_theta - X_N1[n +0] * sin_theta;
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

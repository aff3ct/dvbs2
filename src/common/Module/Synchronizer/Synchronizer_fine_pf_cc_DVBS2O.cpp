#include <cassert>
#include <iostream>

#include "Module/Synchronizer/Synchronizer_fine_pf_cc_DVBS2O.hpp"

// _USE_MATH_DEFINES does not seem to work on MSVC...
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

using namespace aff3ct::module;

template <typename R>
Synchronizer_fine_pf_cc_DVBS2O<R>
::Synchronizer_fine_pf_cc_DVBS2O(const int N, const std::vector<R> pilot_values, const std::vector<int> pilot_start,
                                 const int n_frames)
: Synchronizer<R>(N, N, n_frames),
  pilot_size(pilot_values.size()),
  pilot_nbr(pilot_start.size()),
  pilot_values(pilot_values),
  pilot_start(pilot_start),
  estimated_freq(0)
{
	assert(pilot_size > 0);
	assert(pilot_nbr > 0);
	assert(N > pilot_start[pilot_nbr-1] + pilot_size - 1);
}

template <typename R>
Synchronizer_fine_pf_cc_DVBS2O<R>
::~Synchronizer_fine_pf_cc_DVBS2O()
{}

template <typename R>
void Synchronizer_fine_pf_cc_DVBS2O<R>
::_synchronize(const R *X_N1, R *Y_N2, const int frame_id)
{
	int Lp = this->pilot_size/2;
	R inv_2PI = 1.0f/(2*M_PI);

	std::vector<R> phase_est(this->pilot_nbr, (R)0);
	for (int p = 0; p < this->pilot_nbr; p++)
	{
		std::vector<R> sum_symb_conj_ref(2, (R)0);
		// Phase estimation for every pilot location
		for (int i = 0 ; i < Lp ; i++)
		{
			sum_symb_conj_ref[0] += X_N1[2*this->pilot_start[p] + 2*i    ] * this->pilot_values[2*i    ]
			                      + X_N1[2*this->pilot_start[p] + 2*i + 1] * this->pilot_values[2*i + 1];

			sum_symb_conj_ref[1] += X_N1[2*this->pilot_start[p] + 2*i + 1] * this->pilot_values[2*i    ]
			                      - X_N1[2*this->pilot_start[p] + 2*i    ] * this->pilot_values[2*i + 1];
		}
		phase_est[p] = std::atan2(sum_symb_conj_ref[1], sum_symb_conj_ref[0]);
		phase_est[p] = phase_est[p] < 0 ? phase_est[p] + 2*M_PI : phase_est[p];
	}

	/*if((*this)[syn::tsk::synchronize].is_debug())
	{
	for (int p = 0; p < this->pilot_nbr; p++)
	{
		std::cout << "# {INTERNAL} pilot_start" << p << " = [" << this->pilot_start[p] << std::endl;
		std::vector<R> sum_symb_conj_ref(2, (R)0);
		// Phase estimation for every pilot location
		std::cout << "# {INTERNAL} X_N1_" << p << " = [" << X_N1[this->pilot_start[p]]  << " " << X_N1[this->pilot_start[p] + 1];
		for (int i = 1 ; i < Lp ; i++)
		{
			std::cout << " " << X_N1[this->pilot_start[p] + 2*i]  << " " << X_N1[this->pilot_start[p] + 2*i + 1];
		}
		std::cout << "]"<<std::endl;

		std::cout << "# {INTERNAL} pilots_" << p << " = [" << this->pilot_values[0]  << " " << this->pilot_values[1];
				for (int i = 1 ; i < Lp ; i++)
		{
			std::cout << " " << this->pilot_values[2*i]  << " " << this->pilot_values[2*i+1];
		}
		std::cout << "]"<<std::endl;
	}
	}*/
	std::vector<R> y(this->pilot_nbr,0.0f);
	std::vector<R> t(this->pilot_nbr,0.0f);

	y[0] = inv_2PI*phase_est[0];
	t[0] = this->pilot_start[0] + (R)(Lp/2);

	R acc = (R)0;

	for (int p = 1; p<this->pilot_nbr; p++)
	{
		R diff_angle = phase_est[p] - phase_est[p-1];
		R acc_elt = diff_angle > 0 ? std::floor(diff_angle*inv_2PI + (R)0.5) :  std::ceil(diff_angle*inv_2PI - (R)0.5);
		acc_elt   = std::fabs(diff_angle) > M_PI ? acc_elt : 0.0f;
		acc += acc_elt;
		y[p] = inv_2PI*phase_est[p] - acc;
		t[p] = this->pilot_start[p] + (R)(Lp/2);
	}

	/*if((*this)[syn::tsk::synchronize].is_debug())
	{
		std::cout << "# {INTERNAL} phase_est = [" << phase_est[0] << " ";
		for (int p = 1; p<this->pilot_nbr; p++)
		{
			std::cout << phase_est[p] << " ";
		}
		std::cout << "]"<< std::endl;
	}	*/

	R sum_t   = (R)0;
	R sum_y   = (R)0;
	R sum_ty  = (R)0;
	R sum_tt  = (R)0;

	for (int p = 0; p < this->pilot_nbr; p++)
	{
		sum_t  += t[p];
		sum_y  += y[p];
		sum_ty += t[p]*y[p];
		sum_tt += t[p]*t[p];
	}

	this->estimated_freq  = (this->pilot_nbr * sum_ty - sum_t * sum_y) / (this->pilot_nbr * sum_tt - sum_t * sum_t);
	this->estimated_phase = (sum_y - this->estimated_freq * sum_t) / this->pilot_nbr;

	if((*this)[syn::tsk::synchronize].is_debug())
	{
	/*	std::cout << "# {INTERNAL} t = [" << t[0] << " ";
		for (int p = 1; p<this->pilot_nbr; p++)
		{
			std::cout << t[p] << " ";
		}
		std::cout << "]"<< std::endl;

		std::cout << "# {INTERNAL} y = [" << y[0] << " ";
		for (int p = 1; p<this->pilot_nbr; p++)
		{
			std::cout << y[p] << " ";
		}
		std::cout << "]"<< std::endl;
		*/
		std::cout << "# {INTERNAL} hat_nu = " << this->estimated_freq << " " << std::endl;
		std::cout << "# {INTERNAL} hat_phi = "<< this->estimated_phase << " " << std::endl;
	}

	for (int n = 0 ; n < this->N_in/2 ; n++)
	{
		R theta = 2 * M_PI *(this->estimated_freq * (R)n + this->estimated_phase);

		Y_N2[2*n    ] = X_N1[2*n    ] * std::cos(theta)
		              + X_N1[2*n + 1] * std::sin(theta);

		Y_N2[2*n + 1] = X_N1[2*n + 1] * std::cos(theta)
		              - X_N1[2*n    ] * std::sin(theta);
	}
}

template <typename R>
void Synchronizer_fine_pf_cc_DVBS2O<R>
::reset()
{
	this->estimated_freq = (R)0;
	this->estimated_phase = (R)0;
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Synchronizer_fine_pf_cc_DVBS2O<float>;
template class aff3ct::module::Synchronizer_fine_pf_cc_DVBS2O<double>;
// ==================================================================================== explicit template instantiation

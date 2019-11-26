#include <cassert>
#include <iostream>

#include "Module/Synchronizer/Synchronizer_frame/Synchronizer_frame_DVBS2_fast.hpp"

// _USE_MATH_DEFINES does not seem to work on MSVC...
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

using namespace aff3ct::module;

template <typename R>
Synchronizer_frame_DVBS2_fast<R>
::Synchronizer_frame_DVBS2_fast(const int N, const int n_frames)
: Synchronizer_frame<R>(N, n_frames), reg_channel(std::complex<R>((R)1,(R)0)), corr_vec(N/2, (R)0), output_delay(N, N/2, N/2), corr_SOF(N, conj_SOF), corr_PLSC(N, conj_PLSC), SOF_PLSC_delay(N, 64, 64)
{
}

template <typename R>
Synchronizer_frame_DVBS2_fast<R>
::~Synchronizer_frame_DVBS2_fast()
{}

template <typename R>
void Synchronizer_frame_DVBS2_fast<R>
::_synchronize(const R *X_N1, R *Y_N2, const int frame_id)
{
	int cplx_in_sz = this->N_in/2;

	std::vector<R> cor_SOF         (this->N_in, (R)0);
	std::vector<R> cor_SOF_delayed (this->N_in, (R)0);
	std::vector<R> cor_PLSC        (this->N_in, (R)0);
	std::vector<R> diff_signal     (this->N_in, (R)0);

	diff_signal[0] = this->reg_channel.real() * X_N1[0] + this->reg_channel.imag() * X_N1[1];
	diff_signal[1] = this->reg_channel.imag() * X_N1[0] - this->reg_channel.real() * X_N1[1];
	for (int i = 1 ; i < cplx_in_sz ; i++)
	{
		diff_signal[2*i    ] = X_N1[2*i-2] * X_N1[2*i] + X_N1[2*i-1] * X_N1[2*i+1];
		diff_signal[2*i + 1] = X_N1[2*i-1] * X_N1[2*i] - X_N1[2*i-2] * X_N1[2*i+1];
	}

	corr_SOF      .filter(&diff_signal[0], &cor_SOF [0]       );
	corr_PLSC     .filter(&diff_signal[0], &cor_PLSC[0]       );
	SOF_PLSC_delay.filter(&cor_SOF[0]    , &cor_SOF_delayed[0]);

	R max_corr   = 0;
	int max_idx  = 0;

	for (int i = 0 ; i < cplx_in_sz ; i++)
	{
		R sum_sof_plsc_re = cor_PLSC[2*i    ] + cor_SOF_delayed[2*i    ];
		R sum_sof_plsc_im = cor_PLSC[2*i + 1] + cor_SOF_delayed[2*i + 1];
		R abs2_sum_corr = sum_sof_plsc_re * sum_sof_plsc_re + sum_sof_plsc_im * sum_sof_plsc_im;

		R dif_sof_plsc_re = cor_SOF_delayed[2*i    ] - cor_PLSC[2*i    ];
		R dif_sof_plsc_im = cor_SOF_delayed[2*i + 1] - cor_PLSC[2*i + 1];
		R abs2_dif_corr = dif_sof_plsc_re * dif_sof_plsc_re + dif_sof_plsc_im * dif_sof_plsc_im;

		this->corr_vec[i] = std::sqrt(std::max(abs2_sum_corr, abs2_dif_corr));

		if (this->corr_vec[i] > max_corr)
		{
			max_corr = this->corr_vec[i];
			max_idx  = i;
		}
	}

	this->reg_channel = std::complex<R> (X_N1[2*cplx_in_sz - 2], X_N1[2*cplx_in_sz - 1]);

	this->delay = (cplx_in_sz + max_idx - conj_SOF.size() - conj_PLSC.size()) % cplx_in_sz;
	this->output_delay.set_delay((cplx_in_sz - this->delay)%cplx_in_sz);
	this->output_delay.filter(X_N1,Y_N2);
}

template <typename R>
void Synchronizer_frame_DVBS2_fast<R>
::reset()
{

	this->output_delay.reset();
	this->output_delay.set_delay(0);
	this->reg_channel = std::complex<R>((R)1,(R)0);

	for (size_t i = 0; i < this->corr_vec.size();i++)
		this->corr_vec[i] = (R)0;

	this->delay = 0;
	corr_SOF.reset();
	corr_PLSC.reset();
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Synchronizer_frame_DVBS2_fast<float>;
template class aff3ct::module::Synchronizer_frame_DVBS2_fast<double>;
// ==================================================================================== explicit template instantiation

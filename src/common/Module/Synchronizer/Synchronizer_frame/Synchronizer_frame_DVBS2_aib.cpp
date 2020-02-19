#include <cassert>
#include <iostream>

#include "Module/Synchronizer/Synchronizer_frame/Synchronizer_frame_DVBS2_aib.hpp"

// _USE_MATH_DEFINES does not seem to work on MSVC...
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

using namespace aff3ct::module;

template <typename R>
Synchronizer_frame_DVBS2_aib<R>
::Synchronizer_frame_DVBS2_aib(const int N, const R alpha, const R trigger, const int n_frames)
: Synchronizer_frame<R>(N, n_frames),
  reg_channel(std::complex<R>((R)1,(R)0)),
  sec_SOF_sz(25), sec_PLSC_sz(64),
  corr_buff(89*2, std::complex<R>((R)0,(R)0)),
  corr_vec(N/2, (R)0),
  head(0),
  SOF_PLSC_sz(89),
  output_delay(N, N/2, N/2),
  max_corr((R)0.0f),
  alpha(alpha),
  trigger(trigger)
{
}

template <typename R>
Synchronizer_frame_DVBS2_aib<R>
::~Synchronizer_frame_DVBS2_aib()
{}

template <typename R>
void Synchronizer_frame_DVBS2_aib<R>
::_synchronize(const R *X_N1, int* delay, R *Y_N2, const int frame_id)
{
	int cplx_in_sz = this->N_in/2;
	const std::complex<R>* cX_N1 = reinterpret_cast<const std::complex<R>* > (X_N1);
	std::complex<R>  symb_diff = this->reg_channel * std::conj(cX_N1[0]);
	R new_corr_vec = 0.0f;
	this->step(&symb_diff, &new_corr_vec);
	corr_vec[0] = corr_vec[0]*this->alpha + (1-this->alpha)*new_corr_vec;
	max_corr = corr_vec[0];
	int max_idx  = 0;

	for (auto i = 1; i<cplx_in_sz; i++)
	{
		symb_diff = cX_N1[i-1] * std::conj(cX_N1[i]);

		this->step(&symb_diff, &new_corr_vec);
		corr_vec[i] = corr_vec[i]*this->alpha + (1-this->alpha)*new_corr_vec;

		if (corr_vec[i] > max_corr)
		{
			max_corr = corr_vec[i];
			max_idx  = i;
		}
	}

	if ((*this)[sfm::tsk::synchronize].is_debug())
	{
		std::cout << "# {INTERNAL} CORR = [ ";
		for (auto i = 0; i<cplx_in_sz; i++)
		{
			std::cout << corr_vec[i] << " ";
		}
		std::cout << "]"<< std::endl;
	}

	this->reg_channel = cX_N1[cplx_in_sz - 1];
		//std::cout << "Hi befor delay" << std::endl;
	*delay = (cplx_in_sz + max_idx - this->SOF_PLSC_sz)%cplx_in_sz;
	this->delay = *delay;
	//std::cout << "delay : " << delay<< std::endl;
	this->output_delay.set_delay((cplx_in_sz - *delay)%cplx_in_sz);
	//std::cout << "Delay set" <<std::endl;
	this->output_delay.filter(X_N1,Y_N2);
	//std::cout << "Compute output"<< std::endl;
}

template <typename R>
void Synchronizer_frame_DVBS2_aib<R>
::step(const std::complex<R>* x_elt, R* y_corr)
{
	this->corr_buff[this->head] = *x_elt;
	this->corr_buff[this->head + this->SOF_PLSC_sz] = *x_elt;

	std::complex<R> ps_SOF(0,0);
	for (size_t i = 0; i < this->sec_SOF_sz ; i++)
		ps_SOF += this->corr_buff[this->head+1+i] * this->conj_SOF_PLSC[i];

	std::complex<R> ps_PLSC(0,0);
	for (auto i = this->sec_SOF_sz; i < this->sec_SOF_sz + this->sec_PLSC_sz ; i++)
		ps_PLSC += this->corr_buff[this->head+1+i] * this->conj_SOF_PLSC[i];

	this->head++;
	this->head %= this->SOF_PLSC_sz;

	*y_corr = std::max(std::abs(ps_SOF + ps_PLSC), std::abs(ps_SOF - ps_PLSC));
}


template <typename R>
void Synchronizer_frame_DVBS2_aib<R>
::reset()
{

	this->output_delay.reset();
	this->output_delay.set_delay(0);
	this->reg_channel = std::complex<R>((R)1,(R)0);
	for (size_t i = 0; i < this->corr_buff.size();i++)
		this->corr_buff[i] = std::complex<R>((R)0,(R)0);

	for (size_t i = 0; i < this->corr_vec.size();i++)
		this->corr_vec[i] = (R)0;

	this->head = 0;
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Synchronizer_frame_DVBS2_aib<float>;
template class aff3ct::module::Synchronizer_frame_DVBS2_aib<double>;
// ==================================================================================== explicit template instantiation
#include <cassert>
#include <iostream>
#include <vector>
#include <complex>

#include "Synchronizer_frame_cc_naive.hpp"

// _USE_MATH_DEFINES does not seem to work on MSVC...
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

using namespace aff3ct::module;

template <typename R>
Synchronizer_frame_cc_naive<R>
::Synchronizer_frame_cc_naive(const int N)
: Synchronizer<R>(N,N), reg_channel(std::complex<R>(1,0)),conj_SOF_PLSC(), sec_SOF_sz(25), sec_PLSC_sz(64), corr_buff(89, std::complex<R>(0,0)), head(0), size(89), output_delay(N, N/2, N/2)
{
	this->conj_SOF_PLSC = {std::complex<R>(0,1), std::complex<R>(0,1), std::complex<R>(0,1), std::complex<R>(0,1), std::complex<R>(0,-1), std::complex<R>(0,-1), std::complex<R>(0,-1), std::complex<R>(0,-1), std::complex<R>(0,1), std::complex<R>(0,-1), std::complex<R>(0,-1), std::complex<R>(0,-1), std::complex<R>(0,1), std::complex<R>(0,-1), std::complex<R>(0,-1), std::complex<R>(0,1), std::complex<R>(0,1), std::complex<R>(0,-1), std::complex<R>(0,1), std::complex<R>(0,1), std::complex<R>(0,-1), std::complex<R>(0,1), std::complex<R>(0,-1), std::complex<R>(0,-1), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,1)};
	for (auto i =0; i < 89; i++)
		std::cout << conj_SOF_PLSC[i] << std::endl;
}

template <typename R>
Synchronizer_frame_cc_naive<R>
::~Synchronizer_frame_cc_naive()
{}

template <typename R>
void Synchronizer_frame_cc_naive<R>
::_synchronize(const R *X_N1, R *Y_N2, const int frame_id)
{
	R y_corr = 0;
	R max_corr = 0;
	size_t max_idx;

	const std::complex<R>* cX_N1 = reinterpret_cast<const std::complex<R>* > (X_N1);
	//std::complex<R>* cY_N2       = reinterpret_cast<std::complex<R>*       > (Y_N2);
	
	for (auto i = 0; i<this->N_in/2; i++)
	{
		//std::cout << i << "|" << this->N_in/2 << std::endl;
		this->reg_channel = std::conj(this->reg_channel) * cX_N1[i];
		
		this->step(&reg_channel, &y_corr);
		if (y_corr > max_corr)
		{
			max_corr = y_corr;
			max_idx  = i;
		}
	}
	//std::cout << "Hi befor delay" << std::endl;
	int delay = (max_idx - this->size)%(this->N_in/2);
	//std::cout << "delay : " << delay<< std::endl;
	this->output_delay.set_delay(delay);
	//std::cout << "Delay set" <<std::endl;
	this->output_delay.filter(X_N1,Y_N2);
	//std::cout << "Compute output"<< std::endl;
}

template <typename R>
void Synchronizer_frame_cc_naive<R>
::step(const std::complex<R>* x_elt, R* y_corr)
{
	this->corr_buff[this->head] = *x_elt;
	this->corr_buff[this->head + this->size] = *x_elt;

	std::complex<R> ps_SOF(0,0);
	for (auto i = 0; i < this->sec_SOF_sz ; i++)
		ps_SOF += this->corr_buff[this->head+1+i] * this->conj_SOF_PLSC[i];

	std::complex<R> ps_PLSC(0,0);
	for (auto i = this->sec_SOF_sz; i < this->sec_PLSC_sz ; i++)
		ps_PLSC += this->corr_buff[this->head+1+i] * this->conj_SOF_PLSC[i];	

	this->head++;
	this->head %= this->size;

	*y_corr = std::max(std::abs(ps_SOF+ps_PLSC), std::abs(ps_SOF-ps_PLSC));
}


template <typename R>
void Synchronizer_frame_cc_naive<R>
::reset()
{
} 

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Synchronizer_frame_cc_naive<float>;
template class aff3ct::module::Synchronizer_frame_cc_naive<double>;
// ==================================================================================== explicit template instantiation

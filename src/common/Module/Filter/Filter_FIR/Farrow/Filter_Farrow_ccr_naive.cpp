#include <cassert>
#include <iostream>
#include <vector>
#include <complex>

#include "Module/Filter/Filter_FIR/Farrow/Filter_Farrow_ccr_naive.hpp"

using namespace aff3ct::module;

template <typename R>
Filter_Farrow_ccr_naive<R>
::Filter_Farrow_ccr_naive(const int N, const R mu)
: Filter<R>(N,N), xnd2_1(std::complex<R>((R)0,(R)0)), xnd2_2(std::complex<R>((R)0,(R)0)), xnd2_3(std::complex<R>((R)0,(R)0)), xn_1(std::complex<R>((R)0,(R)0)), xn_2(std::complex<R>((R)0,(R)0)), mu(mu)
{
}

template <typename R>
Filter_Farrow_ccr_naive<R>
::~Filter_Farrow_ccr_naive()
{}

template <typename R>
inline void Filter_Farrow_ccr_naive<R>
::step(const std::complex<R>* x_elt, std::complex<R>* y_elt)
{
	std::complex<R> xnd2 = *x_elt * std::complex<R>(0.5f,0.0f);

	*y_elt  = this->xn_2 + this->mu * ((-xnd2 + this->xn_1 + this->xnd2_1 - this->xnd2_2 - this->xnd2_3)
	                                 + ( xnd2              - this->xnd2_1 - this->xnd2_2 + this->xnd2_3) * this->mu);

	this->xn_2 = this->xn_1; 
	this->xn_1 = *x_elt;
	
	this->xnd2_3 = this->xnd2_2;
	this->xnd2_2 = this->xnd2_1;
	this->xnd2_1 = xnd2;
}

template <typename R>
void Filter_Farrow_ccr_naive<R>
::_filter(const R *X_N1,  R *Y_N2, const int frame_id)
{
	auto cX_N1 = reinterpret_cast<const std::complex<R>* >(X_N1);
	auto cY_N2 = reinterpret_cast<std::complex<R>* >(Y_N2);

	for (auto i=0; i < this->N/2 ; i++)
		this->step(&cX_N1[i], &cY_N2[i]);
}

template <typename R>
void Filter_Farrow_ccr_naive<R>
::set_mu(const R mu)
{
	this->mu = mu;
}

template <typename R>
void Filter_Farrow_ccr_naive<R>
::reset()
{
	this->xn_2 = std::complex<R>(R(0),R(0)); 
	this->xn_1 = std::complex<R>(R(0),R(0));
	
	this->xnd2_3 = std::complex<R>(R(0),R(0));
	this->xnd2_2 = std::complex<R>(R(0),R(0));
	this->xnd2_1 = std::complex<R>(R(0),R(0));
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Filter_Farrow_ccr_naive<float>;
template class aff3ct::module::Filter_Farrow_ccr_naive<double>;
// ==================================================================================== explicit template instantiation

#include <cassert>
#include <iostream>
#include <vector>
#include <complex>

#include "Module/Filter/Filter_FIR/Farrow/Filter_Farrow_ccr_naive.hpp"

using namespace aff3ct::module;

template <typename R>
Filter_Farrow_ccr_naive<R>
::Filter_Farrow_ccr_naive(const int N, const R mu, const int n_frames)
: Filter_FIR_ccr<R>(N, std::vector<R>(4,(R)0.0), n_frames), mu(mu)
{
	this->set_mu(mu);
}

template <typename R>
Filter_Farrow_ccr_naive<R>
::~Filter_Farrow_ccr_naive()
{}

template <typename R>
void Filter_Farrow_ccr_naive<R>
::set_mu(const R mu)
{
	this->mu = mu;
	this->b[3] = (R)0.5 * mu * mu - (R)0.5 * mu;
	this->b[2] = (R)1.5 * mu - (R)0.5 * mu * mu;
	this->b[1] = (R)1.0 - (R)0.5*mu - (R)0.5 * mu * mu;
	this->b[0] = (R)0.5 * mu * mu - (R)0.5 * mu;
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Filter_Farrow_ccr_naive<float>;
template class aff3ct::module::Filter_Farrow_ccr_naive<double>;
// ==================================================================================== explicit template instantiation
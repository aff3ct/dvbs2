#include <cassert>
#include <iostream>
#include <vector>
#include <complex>

#include "Module/Filter/Filter_FIR/Filter_FIR_ccr.hpp"
using namespace aff3ct::module;

template <typename R>
Filter_FIR_ccr<R>
::Filter_FIR_ccr(const int N, const std::vector<R> b)
: Filter<R>(N,N),
b(b.size(), R(0)),
buff(2*b.size(), std::complex<R>(R(0))),
head(0),
size((int)b.size()),
M(mipp::N<R>()),
P((N-2*(b.size()-1))/mipp::N<R>())
{
	assert(size > 0);
	assert(mipp::N<R>() > 1);

	if(P < 0)
		P = 0;

	for (size_t i = 0; i < b.size(); i++)
		this->b[i] = b[b.size() - 1 - i];
}

template <typename R>
Filter_FIR_ccr<R>
::~Filter_FIR_ccr()
{}

template <typename R>
void Filter_FIR_ccr<R>
::_filter(const R *X_N1, R *Y_N2, const int frame_id)
{
	auto cX_N1 = reinterpret_cast<const std::complex<R>* >(X_N1);
	auto cY_N2 = reinterpret_cast<      std::complex<R>* >(Y_N2);

	int rest = this->N - this->P * this->M;

	for(auto i = 0; i < rest/2; i++)
		step(&cX_N1[i], &cY_N2[i]);

	mipp::Reg<R> ps;
	mipp::Reg<R> reg_x;
	mipp::Reg<R> reg_b;
	for(auto i = rest ; i<this->N ; i+=this->M)
	{
		ps = (R)0;
		for(int k = 0; k < b.size() ; k++)
		{
			reg_b = b[k];
			reg_x.load(X_N1 + i - 2*(b.size() - 1 - k));
			ps += reg_b * reg_x;
		}
		ps.store(Y_N2 + i);
	}
	int sz = this->N/2;
	std::copy(&cX_N1[sz - this->size], &cX_N1[sz], &this->buff[0]);
	std::copy(&cX_N1[sz - this->size], &cX_N1[sz], &this->buff[this->size]);
	this->head = 0;
}


template <typename R>
void Filter_FIR_ccr<R>
::reset()
{
	for (size_t i = 0; i<this->buff.size(); i++)
		this->buff[i] = std::complex<R>(R(0),R(0));

	this->head = 0;
}

template <typename R>
std::vector<R> Filter_FIR_ccr<R>
::get_filter_coefs()
{
	std::vector<R> flipped_b(this->b.size(),(R)0);
	for (size_t i=0; i<this->b.size();i++)
		flipped_b[i] = this->b[this->b.size()-1-i];

	return flipped_b;
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Filter_FIR_ccr<float>;
template class aff3ct::module::Filter_FIR_ccr<double>;
// ==================================================================================== explicit template instantiation

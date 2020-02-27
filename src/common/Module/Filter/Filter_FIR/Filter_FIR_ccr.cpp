#include <cassert>
#include <iostream>
#include <vector>
#include <complex>

#include "Module/Filter/Filter_FIR/Filter_FIR_ccr.hpp"
using namespace aff3ct::module;

template <typename R>
Filter_FIR_ccr<R>
::Filter_FIR_ccr(const int N, const std::vector<R> b, const int n_frames)
: Filter<R>(N, N, n_frames),
buff(2*b.size(), std::complex<R>(R(0))),
head(0),
size((int)b.size()),
M(mipp::N<R>()),
P((N-2*(b.size()-1))/mipp::N<R>()),
b(b.size(), R(0))
{
	assert(size > 0);
	assert(mipp::N<R>() > 1);

	if (P < 0)
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
::_filter_old(const R *X_N1, R *Y_N2, const int frame_id)
{
	auto cX_N1 = reinterpret_cast<const std::complex<R>* >(X_N1);
	auto cY_N2 = reinterpret_cast<      std::complex<R>* >(Y_N2);

	int rest = this->N - this->P * this->M;

	for(auto i = 0; i < rest/2; i++)
		step(&cX_N1[i], &cY_N2[i]);

	mipp::Reg<R> ps;
	mipp::Reg<R> reg_x;
	mipp::Reg<R> reg_b;
	size_t b_size = b.size();
	for(auto i = rest ; i < this->N ; i += this->M)
	{
		ps = (R)0;
		for(size_t k = 0; k < b_size ; k++)
		{
			reg_b = b[k];
			reg_x.load(X_N1 + i - 2*(b_size - 1 - k));
			ps = mipp::fmadd(reg_b, reg_x, ps); // same as 'ps += reg_b * reg_x'
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
::_filter(const R *X_N1, R *Y_N2, const int frame_id)
{
	auto cX_N1 = reinterpret_cast<const std::complex<R>* >(X_N1);
	auto cY_N2 = reinterpret_cast<      std::complex<R>* >(Y_N2);

	int rest = this->N - this->P * this->M;

	for(auto i = 0; i < rest/2; i++)
		step(&cX_N1[i], &cY_N2[i]);

	mipp::Reg<R> ps = (R)0;
	mipp::Reg<R> ps0;
	mipp::Reg<R> ps1;
	mipp::Reg<R> ps2;
	mipp::Reg<R> ps3;

	mipp::Reg<R> reg_x0;
	mipp::Reg<R> reg_x1;
	mipp::Reg<R> reg_x2;
	mipp::Reg<R> reg_x3;

	mipp::Reg<R> reg_b0;
	mipp::Reg<R> reg_b1;
	mipp::Reg<R> reg_b2;
	mipp::Reg<R> reg_b3;

	size_t b_size = b.size();
	size_t b_size_unrolled4 = (b_size / 4) * 4;

	for (auto i = rest; i < this->N ; i += this->M)
	{
		ps0 = (R)0;
		ps1 = (R)0;
		ps2 = (R)0;
		ps3 = (R)0;

		for (size_t k = 0; k < b_size_unrolled4; k += 4)
		{
			reg_b0 = b[k +0];
			reg_b1 = b[k +1];
			reg_b2 = b[k +2];
			reg_b3 = b[k +3];

			reg_x0 = &X_N1[-2 * (b_size -1) + i + 2 * (k + 0)];
			reg_x1 = &X_N1[-2 * (b_size -1) + i + 2 * (k + 1)];
			reg_x2 = &X_N1[-2 * (b_size -1) + i + 2 * (k + 2)];
			reg_x3 = &X_N1[-2 * (b_size -1) + i + 2 * (k + 3)];

			ps0 = mipp::fmadd(reg_b0, reg_x0, ps0); // same as 'ps0 += reg_b0 * reg_x0'
			ps1 = mipp::fmadd(reg_b1, reg_x1, ps1); // same as 'ps1 += reg_b1 * reg_x1'
			ps2 = mipp::fmadd(reg_b2, reg_x2, ps2); // same as 'ps2 += reg_b2 * reg_x2'
			ps3 = mipp::fmadd(reg_b3, reg_x3, ps3); // same as 'ps3 += reg_b3 * reg_x3'
		}

		ps0 += ps1;
		ps2 += ps3;
		ps = ps0 + ps2;

		for (size_t k = b_size_unrolled4; k < b_size; k++)
		{
			reg_b0 = b[k];
			reg_x0.load(X_N1 - 2 * (b_size -1) + i + (2 * k));
			ps = mipp::fmadd(reg_b0, reg_x0, ps); // same as 'ps += reg_b0 * reg_x0'
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
	for (size_t i = 0; i < this->buff.size(); i++)
		this->buff[i] = std::complex<R>(R(0),R(0));

	this->head = 0;
}

template <typename R>
std::vector<R> Filter_FIR_ccr<R>
::get_filter_coefs()
{
	std::vector<R> flipped_b(this->b.size(),(R)0);
	for (size_t i=0; i < this->b.size(); i++)
		flipped_b[i] = this->b[this->b.size()-1-i];

	return flipped_b;
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Filter_FIR_ccr<float>;
template class aff3ct::module::Filter_FIR_ccr<double>;
// ==================================================================================== explicit template instantiation

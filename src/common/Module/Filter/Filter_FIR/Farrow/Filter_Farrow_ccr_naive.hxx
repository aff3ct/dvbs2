#include "Module/Filter/Filter_FIR/Farrow/Filter_Farrow_ccr_naive.hpp"

namespace aff3ct
{
namespace module
{

template <typename R>
void Filter_Farrow_ccr_naive<R>
::set_mu(const R mu)
{
	this->mu = mu;
	auto half_mu        = (R)0.5 * mu;
	auto half_mu_square = half_mu * mu;

	this->b[0] = half_mu_square - half_mu;
	this->b[1] = (R)1.0 - half_mu - half_mu_square;
	this->b[2] = mu + half_mu - half_mu_square;
	this->b[3] = this->b[0];
}

template <typename R>
void Filter_Farrow_ccr_naive<R>
::step(const std::complex<R>* x_elt, std::complex<R>* y_elt)
{
	auto X     = reinterpret_cast<const R*>(x_elt            );
	auto Y     = reinterpret_cast<      R*>(y_elt            );
	auto buff2 = reinterpret_cast<      R*>(this->buff.data());

	auto hx2 = this->head*2;

	buff2[hx2 +0] = X[0];
	buff2[hx2 +1] = X[1];
	buff2[hx2 +8] = X[0];
	buff2[hx2 +9] = X[1];

	// Y[0] = 0;
	// Y[1] = 0;
	// for (auto i = 0; i < 4; i++)
	// {
	// 	Y[0] += buff2[this->head*2 + 2 + 2 * i +0] * this->b[i];
	// 	Y[1] += buff2[this->head*2 + 2 + 2 * i +1] * this->b[i];
	// }

	auto r0 = buff2[hx2 +2] * this->b[0];
	auto i0 = buff2[hx2 +3] * this->b[0];
	auto r1 = buff2[hx2 +4] * this->b[1];
	auto i1 = buff2[hx2 +5] * this->b[1];
	auto r2 = buff2[hx2 +6] * this->b[2];
	auto i2 = buff2[hx2 +7] * this->b[2];
	auto r3 = buff2[hx2 +8] * this->b[3];
	auto i3 = buff2[hx2 +9] * this->b[3];

	r0 += r1;
	r2 += r3;
	i0 += i1;
	i2 += i3;

	Y[0] = r0 + r2;
	Y[1] = i0 + i2;

	this->head++;
	this->head %= 4;
}

}
}

#ifndef FILTER_UPFIR_CCR_NAIVE_HPP
#define FILTER_UPFIR_CCR_NAIVE_HPP

#include <vector>
#include <complex>
#include "../Filter_FIR/Filter_FIR_ccr_naive.hpp"
#include "../Filter.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Filter_UPFIR_ccr_naive : public Filter<R>
{	
public:
	Filter_UPFIR_ccr_naive(const int N, const std::vector<R> H, const int F = 1);
	virtual ~Filter_UPFIR_ccr_naive();
	inline void step  (const std::complex<R>* x_elt, std::complex<R>* y_elts);
	void reset();

protected:
	void _filter(const R *X_N1, R *Y_N2, const int frame_id);

private:
	const int                           F;       // Upsampling Factor
	std::vector<std::vector<R> >        H;       // Impulse response
	std::vector<Filter_FIR_ccr_naive<R> > flt_bank;// Filter bank
};


template <typename R>
Filter_UPFIR_ccr_naive<R>
::Filter_UPFIR_ccr_naive(const int N, std::vector<R> H, const int F)
	: Filter<R>(N, F*N), F(F), H(F), flt_bank()
{
	this->H.resize(F);
	for(auto i=0; i < H.size(); i++)
		this->H[i%F].push_back(H[i]);

	for (auto f=0;f<F; f++)
		this->flt_bank.push_back(Filter_FIR_ccr_naive<R>(N,this->H[f]));
}

template <typename R>
Filter_UPFIR_ccr_naive<R>
::~Filter_UPFIR_ccr_naive()
{
}

template <typename R>
void Filter_UPFIR_ccr_naive<R>
::step (const std::complex<R>* x_elt, std::complex<R>* y_F)
{
	for (auto f = 0; f<this->F; f++)
		this->flt_bank[f].step(x_elt, y_F + f);
}

template <typename R>
void Filter_UPFIR_ccr_naive<R>
::reset()
{
	for (auto f = 0; f<this->F; f++)
		this->flt_bank[f].reset();
}

template <typename R>
void Filter_UPFIR_ccr_naive<R>
::_filter(const R *X_N1, R *Y_N2, const int frame_id)
{
	auto cX_N1 = reinterpret_cast<const std::complex<R>* >(X_N1);
	auto cY_N2 = reinterpret_cast<std::complex<R>* >(Y_N2);

	for (auto f = 0; f<this->F; f++)
		for (auto i = 0; i < this->N/2; i++)
			this->flt_bank[f].step(cX_N1 + i, cY_N2 + i*this->F + f);
}
}
}
#endif //FILTER_UPFIR_CCR_NAIVE_HPP

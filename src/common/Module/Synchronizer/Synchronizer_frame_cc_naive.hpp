#ifndef SYNCHRONIZER_FRAME_CC_NAIVE_HPP
#define SYNCHRONIZER_FRAME_CC_NAIVE_HPP

#include <vector>
#include <complex>

#include "Module/Synchronizer/Synchronizer.hpp"
#include "Module/Filter/Variable_delay/Variable_delay_cc_naive.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Synchronizer_frame_cc_naive : public Synchronizer<R>
{
private:
	const std::vector<std::complex<R> >  conj_SOF_PLSC = {std::complex<R>(0,1), std::complex<R>(0,1), std::complex<R>(0,1), std::complex<R>(0,1), std::complex<R>(0,-1), std::complex<R>(0,-1), std::complex<R>(0,-1), std::complex<R>(0,-1), std::complex<R>(0,1), std::complex<R>(0,-1), std::complex<R>(0,-1), std::complex<R>(0,-1), std::complex<R>(0,1), std::complex<R>(0,-1), std::complex<R>(0,-1), std::complex<R>(0,1), std::complex<R>(0,1), std::complex<R>(0,-1), std::complex<R>(0,1), std::complex<R>(0,1), std::complex<R>(0,-1), std::complex<R>(0,1), std::complex<R>(0,-1), std::complex<R>(0,-1), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,1)};
	std::complex            <R>   reg_channel;

	const size_t sec_SOF_sz;
	const size_t sec_PLSC_sz;

	std::vector<std::complex<R> > corr_buff;
	std::vector<R               > corr_vec;
	int head;
	int SOF_PLSC_sz;
	int delay;
	Variable_delay_cc_naive<R> output_delay;

public:
	Synchronizer_frame_cc_naive (const int N);
	virtual ~Synchronizer_frame_cc_naive();
	void step(const std::complex<R>* x_elt, R* y_elt);
	void reset();
	int get_delay(){return this->delay;};
protected:
	void _synchronize(const R *X_N1,  R *Y_N2, const int frame_id);

};

}
}

#endif //SYNCHRONIZER_FRAME_CC_NAIVE_HPP

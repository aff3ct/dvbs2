#ifndef SYNCHRONIZER_FRAME_DVBS2_FAST_HPP
#define SYNCHRONIZER_FRAME_DVBS2_FAST_HPP

#include <vector>
#include <complex>

#include "Module/Synchronizer/Synchronizer_frame/Synchronizer_frame.hpp"
#include "Module/Filter/Variable_delay/Variable_delay_cc_naive.hpp"
#include "Module/Filter/Filter_FIR/Filter_FIR_ccr.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Synchronizer_frame_DVBS2_fast : public Synchronizer_frame<R>
{
private:
	const std::vector<R>  conj_SOF = {(R) 1, (R)-1, (R)-1, (R) 1, (R)-1,
	                                  (R) 1, (R) 1, (R)-1, (R) 1, (R) 1,
	                                  (R)-1, (R)-1, (R) 1, (R)-1, (R)-1,
	                                  (R)-1, (R) 1, (R)-1, (R)-1, (R)-1,
	                                  (R)-1, (R) 1, (R) 1, (R) 1, (R) 1};

	const std::vector<R>  conj_PLSC = { (R) 1, (R) 0, (R) 1, (R) 0, (R)-1, (R) 0, (R)-1, (R) 0,
	                                    (R) 1, (R) 0, (R)-1, (R) 0, (R) 1, (R) 0, (R)-1, (R) 0,
	                                    (R) 1, (R) 0, (R)-1, (R) 0, (R)-1, (R) 0, (R) 1, (R) 0,
	                                    (R)-1, (R) 0, (R)-1, (R) 0, (R) 1, (R) 0, (R) 1, (R) 0,
	                                    (R) 1, (R) 0, (R) 1, (R) 0, (R)-1, (R) 0, (R)-1, (R) 0,
	                                    (R)-1, (R) 0, (R)-1, (R) 0, (R)-1, (R) 0, (R) 1, (R) 0,
	                                    (R) 1, (R) 0, (R)-1, (R) 0, (R) 1, (R) 0, (R) 1, (R) 0,
	                                    (R) 1, (R) 0, (R)-1, (R) 0, (R)-1, (R) 0, (R) 1, (R) 0};

	std::complex <R>   reg_channel;
	std::vector<R               > corr_vec;
	Variable_delay_cc_naive<R> output_delay;

	Filter_FIR_ccr<R>    corr_SOF;
	Filter_FIR_ccr<R>    corr_PLSC;
	Variable_delay_cc_naive<R> SOF_PLSC_delay;

	std::vector<R> cor_SOF;
	std::vector<R> cor_SOF_delayed;
	std::vector<R> cor_PLSC;
	std::vector<R> diff_signal;

	R alpha;
	R max_corr;
	R trigger;
	// std::vector<R> cor_PLSC_re;
	// std::vector<R> cor_PLSC_im;
	// std::vector<R> cor_SOF_delayed_re;
	// std::vector<R> cor_SOF_delayed_im;

public:
	Synchronizer_frame_DVBS2_fast (const int N, const R alpha = 0, const R trigger = 25, const int n_frames = 1);
	virtual ~Synchronizer_frame_DVBS2_fast();
	void step(const std::complex<R>* x_elt, R* y_elt);
	void reset();
	R _get_metric() const {return this->max_corr;};
	bool _get_packet_flag() const {return(this->max_corr > this->trigger);};

protected:
	void _synchronize(const R *X_N1, int* delay, R *Y_N2, const int frame_id);
};

}
}

#endif //SYNCHRONIZER_FRAME_DVBS2_FAST_HPP

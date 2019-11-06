#ifndef SYNCHRONIZER_GARDNER_CC_NAIVE_HPP
#define SYNCHRONIZER_GARDNER_CC_NAIVE_HPP

#include <vector>
#include <complex>

#include "Module/Synchronizer/Synchronizer.hpp"
#include "Module/Filter/Filter_FIR/Farrow/Filter_Farrow_ccr_naive.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Synchronizer_Gardner_cc_naive : public Synchronizer<R>
{
private:
	const std::vector<int>  set_bits_nbr = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};

	const int osf;
	const int POW_osf;
	const R   INV_osf;

	std::complex<R> last_symbol;

	// Interpolation parametes
	R mu;
	Filter_Farrow_ccr_naive <R>   farrow_flt;

	// TED parameters
	int strobe_history;
	int is_strobe;
	R TED_error;
	std::vector<std::complex<R> > TED_buffer;
	int TED_head_pos;
	int TED_mid_pos;

	// Loop filter parameters
	R lf_proportional_gain; // AIB -1.6666e-05; //  | -0.002951146572088;    // Matlab default
	R lf_integrator_gain; // AIB-2.7777e-10; // | -5.902293144176643e-06;// Matlab default
	R lf_prev_in;
	R lf_filter_state;
	R lf_output;

	R NCO_counter;

	int overflow_cnt;
	int underflow_cnt;
	std::vector<std::complex<R> > output_buffer;

	int outbuf_head;
	int outbuf_tail;
	int outbuf_max_sz;
	int outbuf_cur_sz;

	void TED_update(std::complex<R> strobe);
	void loop_filter();
	void interpolation_control();
	void push(const std::complex<R> strobe);

public:
	Synchronizer_Gardner_cc_naive (const int N, int osf, const R damping_factor = std::sqrt(0.5), const R normalized_bandwidth = (R)5e-5, const R detector_gain = (R)2);
	virtual ~Synchronizer_Gardner_cc_naive();
	void reset();

	void step(const std::complex<R> *X_N1);
	R get_mu() {return this->mu;};
	int get_is_strobe() {return this->is_strobe;};
	std::complex<R> get_last_symbol() {return this->last_symbol;};
	void pop(std::complex<R> *strobe);
	int get_overflow_cnt (){return this->overflow_cnt;};
	int get_underflow_cnt (){return this->underflow_cnt;};
	int get_delay();
	void set_loop_filter_coeffs(const R damping_factor, const R normalized_bandwidth, const R detector_gain);

protected:
	void _synchronize(const R *X_N1,  R *Y_N2, const int frame_id);

};

}
}

#endif //SYNCHRONIZER_GARDNER_CC_NAIVE_HPP

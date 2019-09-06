#ifndef SYNCHRONIZER_GARDNER_CC_NAIVE_HPP
#define SYNCHRONIZER_GARDNER_CC_NAIVE_HPP

#include <vector>
#include <complex>

#include "Synchronizer.hpp"
#include "../Filter/Filter_FIR/Farrow/Filter_Farrow_ccr_naive.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Synchronizer_Gardner_cc_naive : public Synchronizer<R>
{
private:
	const int OSF;
	const int POW_OSF;
	const R   INV_OSF;

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
	int TED_old_head_pos;
	const std::vector<int>  set_bits_nbr;

	// Loop filter parameters
	R lf_proportional_gain; // AIB -1.6666e-05; //  | -0.002951146572088;    // Matlab default
	R lf_integrator_gain; // AIB-2.7777e-10; // | -5.902293144176643e-06;// Matlab default
	R lf_prev_in;
	R lf_filter_state;
	R lf_output;

	R NCO_counter;

	void TED_update(std::complex<R> strobe);
	void loop_filter();
	void interpolation_control();
	

public:
	Synchronizer_Gardner_cc_naive (const int N, int OSF);
	virtual ~Synchronizer_Gardner_cc_naive();
	void reset();

	void step(const std::complex<R> *X_N1);
	R get_mu() {return this->mu;};
	int get_is_strobe() {return this->is_strobe;};
	std::complex<R> get_last_symbol() {return this->last_symbol;};
	
protected:
	void _synchronize(const R *X_N1,  R *Y_N2, const int frame_id);

};

}
}

#endif //SYNCHRONIZER_GARDNER_CC_NAIVE_HPP

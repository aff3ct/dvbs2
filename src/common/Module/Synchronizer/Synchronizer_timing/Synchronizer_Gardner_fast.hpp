#ifndef SYNCHRONIZER_GARDNER_FAST_HPP
#define SYNCHRONIZER_GARDNER_FAST_HPP

#include <vector>
#include <complex>

#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_timing.hpp"
#include "Module/Filter/Filter_FIR/Farrow/Filter_Farrow_ccr_naive.hpp"

namespace aff3ct
{
namespace module
{
template <typename B=int, typename R = float>
class Synchronizer_Gardner_fast : public Synchronizer_timing<B,R>
{
private:
	const std::vector<int>  set_bits_nbr = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};

	// Interpolation filter
	Filter_Farrow_ccr_naive <R>   farrow_flt;

	// TED parameters
	int strobe_history;
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

public:
	Synchronizer_Gardner_fast (const int N, int osf, const R damping_factor = std::sqrt(0.5), const R normalized_bandwidth = (R)5e-5, const R detector_gain = (R)2, const int n_frames = 1);
	virtual ~Synchronizer_Gardner_fast();


	inline void step(const std::complex<R> *X_N1, std::complex<R>* Y_N1, B* B_N1);

	void set_loop_filter_coeffs(const R damping_factor,
	                            const R normalized_bandwidth,
	                            const R detector_gain        );

protected:
	void _reset();
	void _synchronize(const R *X_N1, R *Y_N1, B *B_N1, const int frame_id);
	inline void TED_update(std::complex<R> strobe);
	inline void loop_filter();
	inline void interpolation_control();
};

}
}

#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_Gardner_fast.hxx"

#endif //SYNCHRONIZER_GARDNER_FAST_HPP

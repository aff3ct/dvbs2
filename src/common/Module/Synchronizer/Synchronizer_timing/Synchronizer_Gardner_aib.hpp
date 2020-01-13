#ifndef SYNCHRONIZER_GARDNER_AIB_HPP
#define SYNCHRONIZER_GARDNER_AIB_HPP

#include <vector>
#include <mutex>
#include <complex>

#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_timing.hpp"
#include "Module/Filter/Filter_FIR/Farrow/Filter_Farrow_ccr_naive.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Synchronizer_Gardner_aib : public Synchronizer_timing<R>
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

	std::mutex buffer_mtx;
	void TED_update(std::complex<R> strobe);
	void loop_filter();
	void interpolation_control();

public:
	Synchronizer_Gardner_aib (const int N, int osf, const R damping_factor = std::sqrt(0.5), const R normalized_bandwidth = (R)5e-5, const R detector_gain = (R)2, const int n_frames = 1);
	virtual ~Synchronizer_Gardner_aib();


	void step(const std::complex<R> *X_N1);

	void set_loop_filter_coeffs(const R damping_factor,
	                            const R normalized_bandwidth,
	                            const R detector_gain        );

protected:
	void _reset();
	void _synchronize(const R *X_N1,  R *Y_N2, const int frame_id);
	void _sync_push  (const R *X_N1,           const int frame_id);
	void _sync_pull  (                R *Y_N2, const int frame_id);
};

}
}

#endif //SYNCHRONIZER_GARDNER_AIB_HPP

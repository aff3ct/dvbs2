#ifndef SYNCHRONIZER_TIMING_PERFECT_HPP
#define SYNCHRONIZER_TIMING_PERFECT_HPP

#include <vector>
#include <complex>

#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_timing.hpp"
#include "Module/Filter/Filter_FIR/Farrow/Filter_Farrow_ccr_naive.hpp"

namespace aff3ct
{
namespace module
{
template <typename B = int, typename R = float>
class Synchronizer_timing_perfect : public Synchronizer_timing<B, R>
{
private:
	// Interpolation filter
	Filter_Farrow_ccr_naive <R>   farrow_flt;

	R NCO_counter_0;
	R NCO_counter;

public:
	Synchronizer_timing_perfect (const int N, const int osf, const R channel_delay, const int n_frames = 1);
	virtual ~Synchronizer_timing_perfect();

	inline void step(const std::complex<R> *X_N1, std::complex<R>* Y_N1, B* B_N1);

protected:
	void _reset();
	void _synchronize(const R *X_N1, R *Y_N1, B *B_N1, const int frame_id);

};

}
}

#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_timing_perfect.hxx"

#endif //SYNCHRONIZER_TIMING_PERFECT_HPP

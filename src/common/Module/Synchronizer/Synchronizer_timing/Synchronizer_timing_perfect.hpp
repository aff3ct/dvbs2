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
template <typename R = float>
class Synchronizer_timing_perfect : public Synchronizer_timing<R>
{
private:
	// Interpolation filter
	Filter_Farrow_ccr_naive <R>   farrow_flt;

	R NCO_counter_0;
	R NCO_counter;

public:
	Synchronizer_timing_perfect (const int N, const int osf, const R channel_delay);
	virtual ~Synchronizer_timing_perfect();

	void reset_();
	void step(const std::complex<R> *X_N1);

protected:
	void _synchronize(const R *X_N1,  R *Y_N2, const int frame_id);

};

}
}

#endif //SYNCHRONIZER_TIMING_PERFECT_HPP
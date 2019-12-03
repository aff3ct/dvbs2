#ifndef SYNCHRONIZER_FREQ_FINE_PERFECT
#define SYNCHRONIZER_FREQ_FINE_PERFECT

#include <vector>
#include <complex>

#include "Module/Synchronizer/Synchronizer_freq/Synchronizer_freq.hpp"
#include "Module/Multiplier/Sine/Multiplier_sine_ccc_naive.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Synchronizer_freq_fine_perfect : public Synchronizer_freq<R>
{
public:
	Synchronizer_freq_fine_perfect(const int N, const R frequency_offset, const R phase_offset, const int n_frames = 1);
	virtual ~Synchronizer_freq_fine_perfect();

protected:
	void _synchronize(const R *X_N1,  R *Y_N2, const int frame_id);
	void _reset ();

};

}
}

#endif //SYNCHRONIZER_FREQ_FINE_PERFECT
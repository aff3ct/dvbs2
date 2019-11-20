#ifndef SYNCHRONIZER_FREQ_COARSE_PERFECT
#define SYNCHRONIZER_FREQ_COARSE_PERFECT

#include <vector>
#include <complex>

#include "Synchronizer_freq_coarse.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Synchronizer_freq_coarse_perfect : public Synchronizer_freq_coarse<R>
{
public:
	Synchronizer_freq_coarse_perfect(const int N);
	virtual ~Synchronizer_freq_coarse_perfect();

	void update_phase(const std::complex<R> spl);
	void step (const std::complex<R>* x_elt, std::complex<R>* y_elt);

	void set_PLL_coeffs (const int pll_sps, const R damping_factor, const R normalized_bandwidth);
	void reset () {};
protected:
	void _synchronize(const R *X_N1,  R *Y_N2, const int frame_id);

};

}
}

#endif //SYNCHRONIZER_FREQ_COARSE_PERFECT
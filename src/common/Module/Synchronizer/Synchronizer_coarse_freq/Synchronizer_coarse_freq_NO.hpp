#ifndef SYNCHRONIZER_COARSE_FR_NO
#define SYNCHRONIZER_COARSE_FR_NO

#include <vector>
#include <complex>

#include "Synchronizer_coarse_freq.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Synchronizer_coarse_freq_NO : public Synchronizer_coarse_freq<R>
{	
public:
	Synchronizer_coarse_freq_NO(const int N);
	virtual ~Synchronizer_coarse_freq_NO();

	void update_phase(const std::complex<R> spl);
	void step (const std::complex<R>* x_elt, std::complex<R>* y_elt);

	void set_PLL_coeffs (const int pll_sps, const R damping_factor, const R normalized_bandwidth);
	void reset () {};
protected:
	void _synchronize(const R *X_N1,  R *Y_N2, const int frame_id);

};

}
}

#endif //SYNCHRONIZER_COARSE_FR_NO

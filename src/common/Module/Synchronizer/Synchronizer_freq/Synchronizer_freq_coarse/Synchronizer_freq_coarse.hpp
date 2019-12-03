#ifndef SYNCHRONIZER_FREQ_COARSE_HPP
#define SYNCHRONIZER_FREQ_COARSE_HPP

#include <vector>
#include <complex>

#include "../Synchronizer_freq.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Synchronizer_freq_coarse : public Synchronizer_freq<R>
{
public:
	Synchronizer_freq_coarse(const int N, const int n_frames = 1);
	virtual ~Synchronizer_freq_coarse() = default;

	virtual void update_phase(const std::complex<R> spl) = 0;
	virtual void step (const std::complex<R>* x_elt, std::complex<R>* y_elt) = 0;
	virtual void set_PLL_coeffs (const int samples_per_symbol, const R damping_factor, const R normalized_bandwidth) = 0;

	void enable_update (){this->is_active = true; };
	void disable_update(){this->is_active = false;};

	void set_curr_idx(int curr_idx) {this->curr_idx = curr_idx;};

protected:
	bool is_active;
	int curr_idx;

	virtual void _synchronize(const R *X_N1,  R *Y_N2, const int frame_id) = 0;
};

}
}
#include "Synchronizer_freq_coarse.hxx"
#endif //SYNCHRONIZER_FREQ_COARSE_HPP

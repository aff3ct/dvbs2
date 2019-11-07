#ifndef SYNCHRONIZER_COARSE_FREQ_HPP
#define SYNCHRONIZER_COARSE_FREQ_HPP

#include <vector>
#include <complex>

#include "../Synchronizer.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Synchronizer_coarse_freq : public Synchronizer<R>
{
public:
	Synchronizer_coarse_freq(const int N, const int n_frames = 1);
	virtual ~Synchronizer_coarse_freq() = default;

	virtual void update_phase(const std::complex<R> spl) = 0;
	virtual void step (const std::complex<R>* x_elt, std::complex<R>* y_elt) = 0;
	virtual void set_PLL_coeffs (const int samples_per_symbol, const R damping_factor, const R normalized_bandwidth) = 0;

	void enable_update (){this->is_active = true; };
	void disable_update(){this->is_active = false;};
	void set_curr_idx(int curr_idx) {this->curr_idx = curr_idx;};

	R get_estimated_freq() {return this->estimated_freq;};

protected:
	bool is_active;
	int curr_idx;
	R estimated_freq;
};

}
}
#include "Synchronizer_coarse_freq.hxx"
#endif //SYNCHRONIZER_COARSE_FREQ_HPP

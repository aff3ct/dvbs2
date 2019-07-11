#ifndef MULTIPLIER_SINE_CCC_NAIVE_HPP
#define MULTIPLIER_SINE_CCC_NAIVE_HPP

#include <cassert>
#include <iostream>
#include <vector>
#include <complex>

#include "../Multiplier.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Multiplier_sine_ccc_naive : public Multiplier<R>
{
private:
	R n;        // Current time instant
	R f;        //Current frequency in Hz
	R omega;    // Current reduced pulsation [0 2pi]
	const R Fs; // Sampling frequency
	
	void _imultiply(const R *X_N,  R *Z_N, const int frame_id);

public:
	Multiplier_sine_ccc_naive (const int N, const R f, const R Fs = 1.0f, const int n_frames = 1);
	void reset_time();
	void set_f (R f);
	R get_nu ();
	
	virtual ~Multiplier_sine_ccc_naive();
	void step      (const std::complex<R>* x_elt, std::complex<R>* y_elt);

};
}
}

#endif //MULTIPLIER_SINE_CCC_NAIVE_HPP

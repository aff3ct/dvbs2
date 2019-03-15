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
class Multiplier_sine_ccc_naive : public Multiplier<float>
{
private:
	float n;        // Current time instant
	float f;        //Current frequency in Hz
	float omega;    // Current reduced pulsation [0 2pi]
	const float Fs; // Sampling frequency
	
	void _imultiply(const float *X_N,  float *Z_N, const int frame_id);

public:
	Multiplier_sine_ccc_naive (const int N, const float f, const float Fs = 1.0f, const int n_frames = 1);
	void reset_time();
	void set_f (float f);
	virtual ~Multiplier_sine_ccc_naive();
	void step      (const std::complex<float>* x_elt, std::complex<float>* y_elt);

};
}
}

#endif //MULTIPLIER_SINE_CCC_NAIVE_HPP
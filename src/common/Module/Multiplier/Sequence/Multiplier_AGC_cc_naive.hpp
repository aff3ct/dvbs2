#ifndef MULTIPLIER_AGC_CC_NAIVE_HPP
#define MULTIPLIER_AGC_CC_NAIVE_HPP

#include <cassert>
#include <iostream>
#include <vector>
#include <complex>

#include "Module/Multiplier/Multiplier.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Multiplier_AGC_cc_naive : public Multiplier<R>
{
private:
	R output_energy;
	void _imultiply(const R *X_N,  R *Z_N, const int frame_id);

public:
	Multiplier_AGC_cc_naive (const int N, const R output_energy = (R)1, const int n_frames = 1);
	virtual ~Multiplier_AGC_cc_naive();
};
}
}

#endif //MULTIPLIER_AGC_CC_NAIVE_HPP

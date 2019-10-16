#ifndef MULTIPLIER_SEQUENCE_CCC_NAIVE_HPP
#define MULTIPLIER_SEQUENCE_CCC_NAIVE_HPP

#include <cassert>
#include <iostream>
#include <vector>
#include <complex>

#include "../Multiplier.hpp"

namespace aff3ct
{
namespace module
{
class Multiplier_sequence_ccc_naive : public Multiplier<float>
{
private:
	std::vector<std::complex<float> > cplx_sequence;
	void _imultiply(const float *X_N,  float *Z_N, const int frame_id);

public:
	Multiplier_sequence_ccc_naive (const int N, const std::vector<float>& sequence, const int n_frames = 1);
	virtual ~Multiplier_sequence_ccc_naive();
};
}
}

#endif //MULTIPLIER_SEQUENCE_CCC_NAIVE_HPP

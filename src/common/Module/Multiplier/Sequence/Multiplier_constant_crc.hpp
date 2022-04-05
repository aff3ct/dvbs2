#ifndef Multiplier_constant_crc_HPP
#define Multiplier_constant_crc_HPP

#include <cassert>
#include <iostream>
#include <vector>
#include <complex>

#include "Module/Multiplier/Multiplier.hpp"

namespace aff3ct
{
namespace module
{
class Multiplier_constant_crc : public Multiplier<float>
{
private:
	float coef;
	void _imultiply(const float *X_N,  float *Z_N, const int frame_id);

public:
	Multiplier_constant_crc (const int N, const float coef, const int n_frames = 1);
	virtual ~Multiplier_constant_crc();
};
}
}

#endif //Multiplier_constant_crc_HPP

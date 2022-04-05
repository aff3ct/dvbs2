#include <cassert>
#include <iostream>
#include <vector>
#include <complex>
#include <cmath>

#include "Module/Multiplier/Sequence/Multiplier_constant_crc.hpp"
using namespace aff3ct::module;

Multiplier_constant_crc
::Multiplier_constant_crc(const int N, const float coef, const int n_frames)
: Multiplier<float>(N, n_frames), coef(coef)
{
}

Multiplier_constant_crc
::~Multiplier_constant_crc()
{}

void Multiplier_constant_crc
::_imultiply(const float *X_N,  float *Z_N, const int frame_id)
{
	for (auto i = 0 ; i < this->N ; i++)
		Z_N[i] = X_N[i] * this->coef;
}

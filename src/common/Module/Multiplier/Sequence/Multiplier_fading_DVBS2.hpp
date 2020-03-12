#ifndef MULTIPLIER_FADING_DVBS2_HPP
#define MULTIPLIER_FADING_DVBS2_HPP

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
class Multiplier_fading_DVBS2 : public Multiplier<R>
{
private:
	std::vector<R> gain_sequence;
	std::vector<int> frame_number;
	int snr_idx;
	int frame_idx;

	void _imultiply(const R *X_N,  R *Z_N, const int frame_id);

public:
	Multiplier_fading_DVBS2 (const int N, const std::string& snr_list_filename, const R snr_ref = 1, const int n_frames = 1);
	virtual ~Multiplier_fading_DVBS2();
	void reset();
};
}
}

#endif //MULTIPLIER_FADING_DVBS2_HPP

#ifndef SYNCHRONIZER_FREQ_COARSE_HXX_
#define SYNCHRONIZER_FREQ_COARSE_HXX_

#include <string>
#include <memory>
#include <stdexcept>
#include <cmath>
#include <sstream>

#include "Tools/Exception/exception.hpp"

namespace aff3ct
{
namespace module
{
	template <typename R>
	Synchronizer_freq_coarse<R>::
	Synchronizer_freq_coarse(const int N, const int n_frames)
	: Synchronizer_freq<R>(N, n_frames), is_active(false), curr_idx(0)
	{
	}
}
}

#endif

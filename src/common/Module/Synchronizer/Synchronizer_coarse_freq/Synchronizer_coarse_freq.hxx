#ifndef SYNCHRONIZER_COARSE_FREQ_HXX_
#define SYNCHRONIZER_COARSE_FREQ_HXX_

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
	Synchronizer_coarse_freq<R>::
	Synchronizer_coarse_freq(const int N)
	: Synchronizer<R>(N,N), is_active(false), curr_idx(0), estimated_freq((R)0.0)
	{
	}
}
}

#endif

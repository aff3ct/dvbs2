#ifndef SYNCHRONIZER_FRAME_HXX_
#define SYNCHRONIZER_FRAME_HXX_

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
	Synchronizer_frame<R>::
	Synchronizer_frame(const int N, const int n_frames)
	: Synchronizer<R>(N, N, n_frames), delay(0)
	{
	}
}
}

#endif

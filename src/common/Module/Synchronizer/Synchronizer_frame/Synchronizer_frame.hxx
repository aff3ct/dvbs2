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
	Synchronizer_frame(const int N)
	: Synchronizer<R>(N,N), delay(0)
	{
	}
}
}

#endif

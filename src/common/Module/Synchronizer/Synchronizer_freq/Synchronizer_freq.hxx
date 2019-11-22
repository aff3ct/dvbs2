#ifndef SYNCHRONIZER_FREQ_HXX_
#define SYNCHRONIZER_FREQ_HXX_

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
	Synchronizer_freq<R>::
	Synchronizer_freq(const int N)
	: Synchronizer<R>(N,N), estimated_freq((R)0.0), estimated_phase((R)0.0)
	{
	}

	template <typename R>
	void Synchronizer_freq<R>
	::reset()
	{
		this->estimated_freq  = (R)0.0;
		this->estimated_phase = (R)0.0;
		this->_reset();
	};
}
}

#endif

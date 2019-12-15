#ifndef SYNCHRONIZER_TIMING_HXX_
#define SYNCHRONIZER_TIMING_HXX_

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
	Synchronizer_timing<R>::
	Synchronizer_timing(const int N, const int osf, const int n_frames)
	: Synchronizer<R>(N, N / osf, n_frames),
	osf(osf),
	POW_osf(1<<osf),
	INV_osf((R)1.0/ (R)osf),
	last_symbol((R)0,(R)0),
	mu((R)0),
	is_strobe(0),
	overflow_cnt(0),
	underflow_cnt(0),
	output_buffer(N/osf, std::complex<R>((R)0,(R)0)),
	outbuf_head  (0),
	outbuf_max_sz(N/osf),
	outbuf_cur_sz(0)
	{
	}

	template <typename R>
	void Synchronizer_timing<R>
	::pull(std::complex<R> *strobe)
	{
		if	(this->outbuf_cur_sz > 0)
		{
			*strobe = this->output_buffer[this->outbuf_tail];
			this->outbuf_tail = (this->outbuf_tail + 1)%this->outbuf_max_sz;
			this->outbuf_cur_sz--;
		}
		else
		{
			// TODO : solve this
			*strobe = std::complex<R>((R)0,(R)0);
			this->underflow_cnt++;
		}
	}


	template <typename R>
	void Synchronizer_timing<R>
	::push(const std::complex<R> strobe)
	{
		if (this->outbuf_cur_sz < this->outbuf_max_sz)
		{
			this->output_buffer[this->outbuf_head] = strobe;
			this->outbuf_head = (this->outbuf_head + 1)%this->outbuf_max_sz;
			this->outbuf_cur_sz++;
		}
		else
		{
			this->overflow_cnt++;
		}
	}

	template <typename R>
	void Synchronizer_timing<R>
	::reset()
	{
		this->last_symbol   = std::complex<R> (R(0),R(0));
		this->is_strobe     = 0;
		this->overflow_cnt  = 0;
		this->underflow_cnt = 0;
		this->outbuf_head   = 0;
		this->outbuf_tail   = 0;
		this->outbuf_cur_sz = 0;

		for (auto i = 0; i<this->outbuf_max_sz ; i++)
			this->output_buffer[i] = std::complex<R>((R)0,(R)0);

		this->_reset();
	};

}
}

#endif

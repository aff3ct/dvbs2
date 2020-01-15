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
: Module(n_frames), N_in(N), N_out(N / osf),
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
	const std::string name = "Synchronizer_timing";
	this->set_name(name);
	this->set_short_name(name);

	if (N_in <= 0)
	{
		std::stringstream message;
		message << "'N_in' has to be greater than 0 ('N_in' = " << N_in << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	if (N_out <= 0)
	{
		std::stringstream message;
		message << "'N_out' has to be greater than 0 ('N_out' = " << N_out << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	auto &p1 = this->create_task("synchronize");
	auto p1s_X_N1 = this->template create_socket_in <R>(p1, "X_N1", this->N_in );
	auto p1s_Y_N2 = this->template create_socket_out<R>(p1, "Y_N2", this->N_out);
	this->create_codelet(p1, [p1s_X_N1, p1s_Y_N2](Module &m, Task &t) -> int
	{
		static_cast<Synchronizer_timing<R>&>(m).synchronize(static_cast<R*>(t[p1s_X_N1].get_dataptr()),
		                                                    static_cast<R*>(t[p1s_Y_N2].get_dataptr()));
		return 0;
	});
}

template <typename R>
int Synchronizer_timing<R>::
get_N_in() const
{
	return this->N_in;
}

template <typename R>
int Synchronizer_timing<R>::
get_N_out() const
{
	return this->N_out;
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

template <typename R>
R Synchronizer_timing<R>
::get_mu()
{
	return this->mu;
}

template <typename R>
std::complex<R> Synchronizer_timing<R>
::get_last_symbol()
{
	return this->last_symbol;
}

template <typename R>
int Synchronizer_timing<R>
::get_is_strobe()
{
	return this->is_strobe;
}

template <typename R>
int Synchronizer_timing<R>
::get_overflow_cnt()
{
	return this->overflow_cnt;
}

template <typename R>
int Synchronizer_timing<R>
::get_underflow_cnt()
{
	return this->underflow_cnt;
}

template <typename R>
int Synchronizer_timing<R>
::get_delay()
{
	return this->outbuf_cur_sz;
}

template <typename R>
bool Synchronizer_timing<R>
::can_pull()
{
	return this->outbuf_cur_sz > this->N_out/2;
}

template <typename R>
template <class AR>
void Synchronizer_timing<R>::
synchronize(const std::vector<R,AR>& X_N1, std::vector<R,AR>& Y_N2, const int frame_id)
{
	if (this->N_in * this->n_frames != (int)X_N1.size())
	{
		std::stringstream message;
		message << "'X_N1.size()' has to be equal to 'N' * 'n_frames' ('X_N1.size()' = " << X_N1.size()
		        << ", 'N' = " << this->N_in << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (this->N_out * this->n_frames != (int)Y_N2.size())
	{
		std::stringstream message;
		message << "'Y_N2.size()' has to be equal to 'N_fil' * 'n_frames' ('Y_N2.size()' = " << Y_N2.size()
		        << ", 'N_fil' = " << this->N_out << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	this->synchronize(X_N1.data(), Y_N2.data(), frame_id);
}

template <typename R>
void Synchronizer_timing<R>::
synchronize(const R *X_N1, R *Y_N2, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_synchronize(X_N1 + f * this->N_in,
		                   Y_N2 + f * this->N_out,
		                   f);
}

template <typename R>
void Synchronizer_timing<R>::
_synchronize(const R *X_N1, R *Y_N2, const int frame_id)
{
	throw tools::unimplemented_error(__FILE__, __LINE__, __func__);
}

}
}

#endif

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
template <typename B, typename R>
Synchronizer_timing<B, R>::
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
output_buffer(N/osf*3,  (R)0),
outbuf_head  (0),
outbuf_max_sz(N/osf*3),
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
	auto &p0 = this->create_task("synchronize");
	auto p0s_X_N1 = this->template create_socket_in <R>(p0, "X_N1", this->N_in );
	auto p0s_Y_N1 = this->template create_socket_out<R>(p0, "Y_N1", this->N_in );
	auto p0s_B_N1 = this->template create_socket_out<B>(p0, "B_N1", this->N_in );
	this->create_codelet(p0, [p0s_X_N1, p0s_Y_N1,p0s_B_N1](Module &m, Task &t) -> int
	{
		static_cast<Synchronizer_timing<B,R>&>(m).synchronize(static_cast<R*>(t[p0s_X_N1].get_dataptr()),
		                                                      static_cast<R*>(t[p0s_Y_N1].get_dataptr()),
		                                                      static_cast<B*>(t[p0s_B_N1].get_dataptr()));
		return 0;
	});

	auto &p1 = this->create_task("push");
	auto p1s_Y_N1 = this->template create_socket_in <R>(p1, "Y_N1", this->N_in );
	auto p1s_B_N1 = this->template create_socket_in <B>(p1, "B_N1", this->N_in );
	this->create_codelet(p1, [p1s_Y_N1,p1s_B_N1](Module &m, Task &t) -> int
	{
		static_cast<Synchronizer_timing<B,R>&>(m).push(static_cast<R*>(t[p1s_Y_N1].get_dataptr()),
		                                               static_cast<B*>(t[p1s_B_N1].get_dataptr()));
		return 0;
	});

	auto &p2 = this->create_task("pull");
	auto p2s_Y_N2 = this->template create_socket_out <R>(p2, "Y_N2", this->N_out );
	this->create_codelet(p2, [p2s_Y_N2](Module &m, Task &t) -> int
	{
		static_cast<Synchronizer_timing<B,R>&>(m).pull(static_cast<R*>(t[p2s_Y_N2].get_dataptr()));
		return 0;
	});
}

template <typename B, typename R>
int Synchronizer_timing<B,R>::
get_N_in() const
{
	return this->N_in;
}

template <typename B, typename R>
int Synchronizer_timing<B,R>::
get_N_out() const
{
	return this->N_out;
}


template <typename B, typename R>
void Synchronizer_timing<B,R>
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
		this->output_buffer[i] = (R)0;

	this->_reset();
};

template <typename B, typename R>
R Synchronizer_timing<B,R>
::get_mu()
{
	return this->mu;
}

template <typename B, typename R>
std::complex<R> Synchronizer_timing<B,R>
::get_last_symbol()
{
	return this->last_symbol;
}

template <typename B, typename R>
int Synchronizer_timing<B,R>
::get_is_strobe()
{
	return this->is_strobe;
}

template <typename B, typename R>
int Synchronizer_timing<B,R>
::get_overflow_cnt()
{
	return this->overflow_cnt;
}

template <typename B, typename R>
int Synchronizer_timing<B,R>
::get_underflow_cnt()
{
	return this->underflow_cnt;
}

template <typename B, typename R>
int Synchronizer_timing<B,R>
::get_delay()
{
	return this->outbuf_cur_sz;
}

template <typename B, typename R>
bool Synchronizer_timing<B,R>
::can_pull()
{
	return this->outbuf_cur_sz > this->N_out;
}

template <typename B, typename R>
template <class AB, class AR>
void Synchronizer_timing<B,R>::
synchronize(const std::vector<R,AR>& X_N1, std::vector<R,AR>& Y_N1, std::vector<B,AB>& B_N1, const int frame_id)
{
	if (this->N_in * this->n_frames != (int)X_N1.size())
	{
		std::stringstream message;
		message << "'X_N1.size()' has to be equal to 'N' * 'n_frames' ('X_N1.size()' = " << X_N1.size()
		        << ", 'N' = " << this->N_in << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (this->N_in * this->n_frames != (int)Y_N1.size())
	{
		std::stringstream message;
		message << "'Y_N1.size()' has to be equal to 'N' * 'n_frames' ('Y_N1.size()' = " << Y_N1.size()
		        << ", 'N' = " << this->N_in << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (this->N_in * this->n_frames != (int)B_N1.size())
	{
		std::stringstream message;
		message << "'B_N1.size()' has to be equal to 'N' * 'n_frames' ('B_N1.size()' = " << B_N1.size()
		        << ", 'N' = " << this->N_in << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	this->synchronize(X_N1.data(), Y_N1.data(), B_N1.data(), frame_id);
}

template <typename B, typename R>
void Synchronizer_timing<B,R>::
synchronize(const R *X_N1, R *Y_N1, B *B_N1, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_synchronize(X_N1 + f * this->N_in, Y_N1 + f * this->N_in, B_N1 + f * this->N_in, f);
}

template <typename B, typename R>
template <class AB, class AR>
void Synchronizer_timing<B,R>::
push(const std::vector<R,AR>& Y_N1, const std::vector<B,AB>& B_N1, const int frame_id)
{
	if (this->N_in * this->n_frames != (int)Y_N1.size())
	{
		std::stringstream message;
		message << "'Y_N1.size()' has to be equal to 'N' * 'n_frames' ('Y_N1.size()' = " << Y_N1.size()
		        << ", 'N' = " << this->N_in << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (this->N_in * this->n_frames != (int)B_N1.size())
	{
		std::stringstream message;
		message << "'B_N1.size()' has to be equal to 'N' * 'n_frames' ('B_N1.size()' = " << B_N1.size()
		        << ", 'N' = " << this->N_in << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	this->push(Y_N1.data(), B_N1.data(), frame_id);
}

template <typename B, typename R>
void Synchronizer_timing<B,R>::
push(const R *Y_N1, const B *B_N1, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_push(Y_N1 + f * this->N_out, B_N1 + f * this->N_out, f);
}

template <typename B, typename R>
template <class AB, class AR>
void Synchronizer_timing<B,R>::
pull(std::vector<R,AR>& Y_N2, const int frame_id)
{
	if (this->N_out * this->n_frames != (int)Y_N2.size())
	{
		std::stringstream message;
		message << "'Y_N2.size()' has to be equal to 'N_fil' * 'n_frames' ('Y_N2.size()' = " << Y_N2.size()
		        << ", 'N_fil' = " << this->N_out << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	this->pull(Y_N2.data(), frame_id);
}

template <typename B, typename R>
void Synchronizer_timing<B,R>::
pull(R *Y_N2, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_pull(Y_N2 + f * this->N_out,f);
}

template <typename B, typename R>
void Synchronizer_timing<B,R>::
_push(const R *Y_N1, const B *B_N1, const int frame_id)
{
	for(auto i = 0; i < this->N_in; i++)
	{
		if (!B_N1[i])
			continue;

		if (this->outbuf_cur_sz < this->outbuf_max_sz)
		{
			this->output_buffer[this->outbuf_head] = Y_N1[i];
			this->outbuf_head = (this->outbuf_head + 1)%this->outbuf_max_sz;
			this->outbuf_cur_sz++;
		}
		else
		{
			this->overflow_cnt++;
		}
	}
}

template <typename B, typename R>
void Synchronizer_timing<B,R>::
_pull(R *Y_N2, const int frame_id)
{
	for(auto i = 0 ; i < this->N_out; i++)
	{
		if	(this->outbuf_cur_sz > 0)
		{
			Y_N2[i] = this->output_buffer[this->outbuf_tail];
			this->outbuf_tail = (this->outbuf_tail + 1)%this->outbuf_max_sz;
			this->outbuf_cur_sz--;
		}
		else
		{
			// TODO : solve this
			Y_N2[i] = (R)0;
			this->underflow_cnt++;
		}
	}
}

}
}

#endif

#include <cassert>
#include <iostream>

#include "Module/Synchronizer/Synchronizer_step_mf_cc.hpp"

// _USE_MATH_DEFINES does not seem to work on MSVC...
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

using namespace aff3ct::module;

template <typename B, typename R>
Synchronizer_step_mf_cc<B,R>
::Synchronizer_step_mf_cc (aff3ct::module::Synchronizer_freq_coarse<R> *sync_coarse_f,
                           aff3ct::module::Filter_RRC_ccr_naive<R>     *matched_filter,
                           aff3ct::module::Synchronizer_timing<B,R>    *sync_timing,
                           const int n_frames)
: Module(n_frames),
  last_delay(0),
  N_in(sync_coarse_f->get_N()),
  N_out(sync_timing->get_N_out()),
  Y_N1_tmp(this->N_in, (R)0),
  B_N1_tmp(this->N_in, (R)0),
  sync_coarse_f(sync_coarse_f),
  matched_filter(matched_filter),
  sync_timing(sync_timing)
{
	const std::string name = "Sync_step_mf";
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
	auto p1s_X_N1  = this->template create_socket_in <R  >(p1, "X_N1" , this->N_in );
	auto p1s_delay = this->template create_socket_in <int>(p1, "delay", 1          );
	auto p1s_Y_N2  = this->template create_socket_out<R  >(p1, "Y_N2" , this->N_out);
	this->create_codelet(p1, [p1s_X_N1, p1s_delay, p1s_Y_N2](Module &m, Task &t) -> int
	{
		static_cast<Synchronizer_step_mf_cc<B,R>&>(m).synchronize(static_cast<R*  >(t[p1s_X_N1 ].get_dataptr()),
		                                                        static_cast<int*>(t[p1s_delay].get_dataptr()),
		                                                        static_cast<R*  >(t[p1s_Y_N2 ].get_dataptr()));

		return 0;
	});
}

template <typename B, typename R>
Synchronizer_step_mf_cc<B,R>
::~Synchronizer_step_mf_cc()
{}

template <typename B, typename R>
int Synchronizer_step_mf_cc<B,R>
::get_N_in() const
{
	return this->N_in;
}

template <typename B, typename R>
int Synchronizer_step_mf_cc<B,R>
::get_N_out() const
{
	return this->N_out;
}

template <typename B, typename R>
int Synchronizer_step_mf_cc<B,R>
::get_delay()
{
	return this->sync_timing->get_delay();
}

template <typename B, typename R>
template <class AB, class AR>
void Synchronizer_step_mf_cc<B,R>
::synchronize(const std::vector<R,AR>& X_N1, const std::vector<int>& delay, std::vector<R,AR>& Y_N2, const int frame_id)
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

template <typename B, typename R>
void Synchronizer_step_mf_cc<B,R>
::synchronize(const R *X_N1, const int* delay, R *Y_N2, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_synchronize(X_N1 + f * this->N_in,
		                   delay + f,
		                   Y_N2 + f * this->N_out,
		                   f);
}

template <typename B, typename R>
void Synchronizer_step_mf_cc<B,R>
::_synchronize(const R *X_N1, const int* delay, R *Y_N2, const int frame_id)
{
	int coarse_delay = (this->N_out - *delay + this->last_delay) % (this->N_out / 2);
	sync_coarse_f->set_curr_idx(coarse_delay);
	this->last_delay = this->sync_timing->get_delay();

	int frame_sps_sz = this->N_in;

	for (int spl_idx = 0; spl_idx < frame_sps_sz/2; spl_idx++)
	{
		std::complex<R> sync_coarse_f_in(X_N1[spl_idx*2], X_N1[spl_idx*2 + 1]);
		std::complex<R> sync_coarse_f_out(0.0f, 0.0f);
		std::complex<R> matched_filter_out(0.0f, 0.0f);

		this->sync_coarse_f ->step (&sync_coarse_f_in,  &sync_coarse_f_out);
		this->matched_filter->step (&sync_coarse_f_out, &matched_filter_out);
		this->sync_timing   ->step (&matched_filter_out, &matched_filter_out, &this->B_N1_tmp[2*spl_idx]);
		this->Y_N1_tmp[2*spl_idx    ] = std::real(matched_filter_out);
		this->Y_N1_tmp[2*spl_idx + 1] = std::imag(matched_filter_out);

		if (this->B_N1_tmp[2*spl_idx] == 1)
			this->sync_coarse_f->update_phase(this->sync_timing->get_last_symbol());
	}
	this->sync_timing->extract(this->Y_N1_tmp.data(), this->B_N1_tmp.data(), Y_N2, 0);
}

template <typename B, typename R>
void Synchronizer_step_mf_cc<B,R>
::reset()
{
	this->sync_coarse_f ->reset();
	this->matched_filter->reset();
	this->sync_timing  ->reset();
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Synchronizer_step_mf_cc<int, float>;
template class aff3ct::module::Synchronizer_step_mf_cc<int, double>;
// ==================================================================================== explicit template instantiation

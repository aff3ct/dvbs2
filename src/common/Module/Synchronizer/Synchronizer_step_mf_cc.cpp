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
: Module(),
  last_delay(0),
  N_in(sync_coarse_f->get_N()),
  N_out(sync_timing->get_N_in()),
  Y_N1_tmp(this->N_in, (R)0),
  B_N1_tmp(this->N_in, (R)0),
  sync_coarse_f(sync_coarse_f),
  matched_filter(matched_filter),
  sync_timing(sync_timing)
{
	const std::string name = "Sync_step_mf";
	this->set_name(name);
	this->set_short_name(name);
	this->set_n_frames(n_frames);
	this->set_single_wave(true);

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
	auto p1s_delay = this->template create_socket_in <int>(p1, "DEL"  , 1          );
	auto p1s_X_N1  = this->template create_socket_in <R  >(p1, "X_N1" , this->N_in );
	auto p1s_MU    = this->template create_socket_out<R  >(p1, "MU"   , 1          );
	auto p1s_FRQ   = this->template create_socket_out<R  >(p1, "FRQ"  , 1          );
	auto p1s_PHS   = this->template create_socket_out<R  >(p1, "PHS"  , 1          );
	auto p1s_Y_N1  = this->template create_socket_out<R  >(p1, "Y_N1" , this->N_out);
	auto p1s_B_N1  = this->template create_socket_out<B  >(p1, "B_N1" , this->N_out);
	this->create_codelet(p1, [p1s_delay, p1s_X_N1, p1s_MU, p1s_FRQ, p1s_PHS, p1s_Y_N1, p1s_B_N1](Module &m, Task &t, const size_t frame_id) -> int
	{
		static_cast<Synchronizer_step_mf_cc<B,R>&>(m).synchronize(static_cast<int*>(t[p1s_delay].get_dataptr()),
		                                                          static_cast<R*  >(t[p1s_X_N1 ].get_dataptr()),
		                                                          static_cast<R*  >(t[p1s_MU   ].get_dataptr()),
		                                                          static_cast<R*  >(t[p1s_FRQ  ].get_dataptr()),
		                                                          static_cast<R*  >(t[p1s_PHS  ].get_dataptr()),
		                                                          static_cast<R*  >(t[p1s_Y_N1 ].get_dataptr()),
		                                                          static_cast<B*  >(t[p1s_B_N1 ].get_dataptr()));

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
::synchronize(const std::vector<int>& DEL, const std::vector<R,AR>& X_N1, std::vector<R,AR>& MU, std::vector<R,AR>& FRQ, std::vector<R,AR>& PHS, std::vector<R,AR>& Y_N1, std::vector<B,AB>& B_N1, const int frame_id)
{
	if (this->n_frames != (int)DEL.size())
	{
		std::stringstream message;
		message << "'DEL.size()' has to be equal to 'n_frames' ('DEL.size()' = " << DEL.size()
		        << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (this->N_in * this->n_frames != (int)X_N1.size())
	{
		std::stringstream message;
		message << "'X_N1.size()' has to be equal to 'N' * 'n_frames' ('X_N1.size()' = " << X_N1.size()
		        << ", 'N' = " << this->N_in << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (this->n_frames != (int)MU.size())
	{
		std::stringstream message;
		message << "'MU.size()' has to be equal to 'n_frames' ('MU.size()' = " << MU.size()
		        << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (this->n_frames != (int)FRQ.size())
	{
		std::stringstream message;
		message << "'FRQ.size()' has to be equal to 'n_frames' ('FRQ.size()' = " << FRQ.size()
		        << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}


	if (this->n_frames != (int)PHS.size())
	{
		std::stringstream message;
		message << "'PHS.size()' has to be equal to 'n_frames' ('PHS.size()' = " << PHS.size()
		        << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (this->N_out * this->n_frames != (int)Y_N1.size())
	{
		std::stringstream message;
		message << "'Y_N1.size()' has to be equal to 'N' * 'n_frames' ('Y_N1.size()' = " << Y_N1.size()
		        << ", 'N' = " << this->N_out << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (this->N_out * this->n_frames != (int)B_N1.size())
	{
		std::stringstream message;
		message << "'B_N1.size()' has to be equal to 'N' * 'n_frames' ('B_N1.size()' = " << B_N1.size()
		        << ", 'N' = " << this->N_out << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}
	this->synchronize(DEL.data(), X_N1.data(), MU.data(), FRQ.data(), PHS.data(), Y_N1.data(), B_N1.data(), frame_id);
}

template <typename B, typename R>
void Synchronizer_step_mf_cc<B,R>
::synchronize(const int* delay, const R *X_N1, R *MU, R *FRQ, R *PHS, R *Y_N1, B *B_N1, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
	{
		this->_synchronize(delay + f,
		                   X_N1 + f * this->N_in,
		                   Y_N1 + f * this->N_out,
						   B_N1 + f * this->N_out,
		                   f);
		MU[f]  = this->sync_timing  ->get_mu();
		FRQ[f] = this->sync_coarse_f->get_estimated_freq();
		PHS[f] = this->sync_coarse_f->get_estimated_phase();
	}
}

template <typename B, typename R>
void Synchronizer_step_mf_cc<B,R>
::_synchronize(const int* delay, const R *X_N1, R *Y_N1, B *B_N1, const int frame_id)
{
	auto cY_N1 = reinterpret_cast<      std::complex<R>*>(Y_N1);

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
		this->sync_timing   ->step (&matched_filter_out, &cY_N1[spl_idx], &B_N1[2*spl_idx]);

		if (B_N1[2*spl_idx] == 1)
			this->sync_coarse_f->update_phase(this->sync_timing->get_last_symbol());
	}
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

#include <cassert>
#include <iostream>

#include "Module/Synchronizer/Synchronizer_step_mf_cc.hpp"

// _USE_MATH_DEFINES does not seem to work on MSVC...
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

using namespace aff3ct::module;

template <typename R>
Synchronizer_step_mf_cc<R>
::	Synchronizer_step_mf_cc (aff3ct::module::Synchronizer_freq_coarse<R>         *sync_coarse_f,
	                         aff3ct::module::Filter_RRC_ccr_naive<R>             *matched_filter,
							 aff3ct::module::Synchronizer_Gardner_cc_naive<R>    *sync_gardner)
: Synchronizer<R>(sync_coarse_f->get_N_in(),sync_gardner->get_N_out()), sync_coarse_f(sync_coarse_f), matched_filter(matched_filter), sync_gardner(sync_gardner)
{
}

template <typename R>
Synchronizer_step_mf_cc<R>
::~Synchronizer_step_mf_cc()
{}

template <typename R>
void Synchronizer_step_mf_cc<R>
::_synchronize(const R *X_N1, R *Y_N2, const int frame_id)
{
	int frame_sym_sz = this->N_out;
	int frame_sps_sz = this->N_in;
	for (int spl_idx = 0; spl_idx < frame_sps_sz/2; spl_idx++)
	{
		std::complex<R> sync_coarse_f_in(X_N1[spl_idx*2], X_N1[spl_idx*2 + 1]);
		std::complex<R> sync_coarse_f_out(0.0f, 0.0f);
		std::complex<R> matched_filter_out(0.0f, 0.0f);

		this->sync_coarse_f ->step (&sync_coarse_f_in,  &sync_coarse_f_out);
		this->matched_filter->step (&sync_coarse_f_out, &matched_filter_out);

		int is_strobe = this->sync_gardner->get_is_strobe();
		this->sync_gardner  ->step (&matched_filter_out);
		if (is_strobe == 1)
			this->sync_coarse_f->update_phase(this->sync_gardner->get_last_symbol());
	}

	auto cY_N2 = reinterpret_cast<std::complex<R>* >(Y_N2);

	for (auto sym_idx = 0 ; sym_idx < frame_sym_sz / 2 ; sym_idx++)
		this->sync_gardner->pop(&cY_N2[sym_idx]);
}

template <typename R>
void Synchronizer_step_mf_cc<R>
::reset()
{
	this->sync_coarse_f ->reset();
	this->matched_filter->reset();
	this->sync_gardner  ->reset();
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Synchronizer_step_mf_cc<float>;
template class aff3ct::module::Synchronizer_step_mf_cc<double>;
// ==================================================================================== explicit template instantiation

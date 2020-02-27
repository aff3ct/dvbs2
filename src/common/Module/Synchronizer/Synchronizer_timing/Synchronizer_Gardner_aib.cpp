#include <cassert>
#include <iostream>

#include "aff3ct.hpp"
#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_Gardner_aib.hpp"

using namespace aff3ct::module;

template <typename B, typename R>
Synchronizer_Gardner_aib<B, R>
::Synchronizer_Gardner_aib(const int N, int osf, const R damping_factor, const R normalized_bandwidth, const R detector_gain, const int n_frames)
: Synchronizer_timing<B,R>(N, osf, n_frames),
farrow_flt(N,(R)0),
strobe_history(0),
TED_error((R)0),
TED_buffer(osf, std::complex<R>((R)0,(R)0)),
TED_head_pos(osf - 1),
TED_mid_pos((osf - 1 - osf / 2) % osf),
lf_proportional_gain((R)0),
lf_integrator_gain   ((R)0),
lf_prev_in ((R)0),
lf_filter_state ((R)0),
lf_output((R)0),
NCO_counter((R)0),
buffer_mtx()
{
	this->set_loop_filter_coeffs(damping_factor, normalized_bandwidth, detector_gain);
}

template <typename B, typename R>
Synchronizer_Gardner_aib<B, R>
::~Synchronizer_Gardner_aib()
{
}

template <typename B, typename R>
void Synchronizer_Gardner_aib<B, R>
::_synchronize(const R *X_N1, R *Y_N1, B *B_N1, const int frame_id)
{
	auto cX_N1 = reinterpret_cast<const std::complex<R>*>(X_N1);
	auto cY_N1 = reinterpret_cast<      std::complex<R>*>(Y_N1);

	for (auto i = 0; i < this->N_in/2 ; i++)
		this->step(&cX_N1[i], &cY_N1[i], &B_N1[2*i]);
}

template <typename B, typename R>
void Synchronizer_Gardner_aib<B, R>
::_reset()
{
	this->mu = (R)0;
	this->farrow_flt.reset();
	this->farrow_flt.set_mu((R)0);

	for (auto i = 0; i<this->osf ; i++)
		this->TED_buffer[i] = std::complex<R>(R(0),R(0));

	this->strobe_history  = 0;
	this->TED_error       = (R)0;
	this->TED_head_pos    = this->osf -1;
	this->TED_mid_pos     = (this->osf - 1 - this->osf / 2) % this->osf;
	this->lf_prev_in      = (R)0;
	this->lf_filter_state = (R)0;
	this->lf_output       = (R)0;
	this->NCO_counter     = (R)0;
}

template <typename B, typename R>
void Synchronizer_Gardner_aib<B, R>
::set_loop_filter_coeffs(const R damping_factor, const R normalized_bandwidth, const R detector_gain)
{
	R K0   = -1;
	R theta = normalized_bandwidth/(R)this->osf/(damping_factor + 0.25/damping_factor);
	R d  = (1 + 2*damping_factor*theta + theta*theta) * K0 * detector_gain;

	this->lf_proportional_gain = (4*damping_factor*theta) /d;
	this->lf_integrator_gain   = (4*theta*theta)/d;
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Synchronizer_Gardner_aib<int, float>;
template class aff3ct::module::Synchronizer_Gardner_aib<int, double>;
// ==================================================================================== explicit template instantiation

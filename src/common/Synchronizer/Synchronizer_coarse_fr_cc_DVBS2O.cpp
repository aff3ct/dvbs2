#include <cassert>
#include <iostream>
#include <vector>
#include <complex>

#include "Synchronizer_coarse_fr_cc_DVBS2O.hpp"
#include "../Multiplier/Sine/Multiplier_sine_ccc_naive.hpp"

// _USE_MATH_DEFINES does not seem to work on MSVC...
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923132169163975144
#endif

using namespace aff3ct::module;

template <typename R>
Synchronizer_coarse_fr_cc_DVBS2O<R>
::Synchronizer_coarse_fr_cc_DVBS2O(const int N, const int samples_per_symbol, const R damping_factor, const R normalized_bandwidth)
:Synchronizer<R>(N,N), scrambled_pilots(this->PL_RAND_SEQ.size(),std::complex<R>((R)0,(R)0)), samples_per_symbol(samples_per_symbol), curr_idx(8369), length_max(8370), proportional_gain((R)1.0), integrator_gain((R)1.0), digital_synthesizer_gain((R)1), prev_spl(std::complex<R>((R)0.0,(R)0.0)), prev_prev_spl(std::complex<R>((R)0.0,(R)0.0)), loop_filter_state((R)0.0),  integ_filter_state((R)0.0), DDS_prev_in((R)0.0), is_active(false), mult(N,(R)0.0, (R)1.0, 1), estimated_freq((R)0.0)
{
	this->set_PLL_coeffs (samples_per_symbol, damping_factor, normalized_bandwidth);
	
	for (auto i = 0; i < this->PL_RAND_SEQ.size(); i++)
	{
		this->scrambled_pilots[i] = std::complex<R>(std::cos((R)M_PI_2*(R)PL_RAND_SEQ[i]), std::sin((R)M_PI_2*(R)PL_RAND_SEQ[i]));
	}
}

template <typename R>
Synchronizer_coarse_fr_cc_DVBS2O<R>
::~Synchronizer_coarse_fr_cc_DVBS2O()
{}

template <typename R>
void Synchronizer_coarse_fr_cc_DVBS2O<R>
::_synchronize(const R *X_N1, R *Y_N2, const int frame_id)
{
	this->mult.imultiply(X_N1, Y_N2, frame_id);
}

template <typename R>
void Synchronizer_coarse_fr_cc_DVBS2O<R>
::step(const std::complex<R>* x_elt, std::complex<R>* y_elt)
{

	this->mult.step(x_elt, y_elt);
}


template <typename R>
void Synchronizer_coarse_fr_cc_DVBS2O<R>
::update_phase(const std::complex<R> spl)
{
	if (this->is_active)
	{
		this->curr_idx = (this->curr_idx + 1)% this->length_max;
		int rem_pos = this->curr_idx % 1476;
		if (rem_pos >= 54 && rem_pos < 90 && this->curr_idx >= 1530)
		{
			int prev_idx = (this->curr_idx - 2)% this->length_max;

			R phase_error = std::imag( spl                 * this->scrambled_pilots[prev_idx      ]
			              * std::conj( this->prev_prev_spl * this->scrambled_pilots[this->curr_idx] )
			                         );

			this->loop_filter_state += phase_error * this->integrator_gain;
                        
			this->integ_filter_state += this->DDS_prev_in ;

			this->DDS_prev_in = phase_error * this->proportional_gain + this->loop_filter_state;
            
			this->estimated_freq = this->digital_synthesizer_gain * this->integ_filter_state / this->samples_per_symbol;
			this->mult.set_nu(-this->estimated_freq);
			
			this->prev_prev_spl = this->prev_spl;
			this->prev_spl = spl;

			//std::cerr << "curr_idx = " << this->curr_idx << " | phase_error = " << phase_error << " | loop_filter_state = " << this->loop_filter_state << " | integ_filter_state = " << this->integ_filter_state << " | nu = "<< -this->digital_synthesizer_gain * this->integ_filter_state / this->samples_per_symbol << std::endl;
		}
		else if (rem_pos == 90 && this->curr_idx >= 1530)
		{
			this->prev_prev_spl = std::complex<R>((R)0.0, (R)0.0);
			this->prev_spl      = std::complex<R>((R)0.0, (R)0.0);			
		}
	}
}

template <typename R>
void Synchronizer_coarse_fr_cc_DVBS2O<R>
::set_PLL_coeffs (const int samples_per_symbol, const R damping_factor, const R normalized_bandwidth)
{
	R phase_error_detector_gain = (R)2.0;
	R phase_recovery_loop_bandwidth = normalized_bandwidth * (R)samples_per_symbol;
	
	//K0
	R phase_recovery_gain = samples_per_symbol;

	R theta = phase_recovery_loop_bandwidth/((damping_factor + 0.25/damping_factor)*samples_per_symbol);
	R d = (R)1.0 + (R)2.0*damping_factor*theta + theta*theta;
	
	//K1
	this->proportional_gain = ((R)4*damping_factor*theta/d)/(phase_error_detector_gain*phase_recovery_gain);
	
	//K2
	this->integrator_gain = ((R)4/samples_per_symbol*theta*theta/d)/(phase_error_detector_gain*phase_recovery_gain);
	
	this->digital_synthesizer_gain = (R)1.0;
}

template <typename R>
void Synchronizer_coarse_fr_cc_DVBS2O<R>
::reset()
{
	this->prev_spl      = std::complex<R> ((R)0.0, (R)0.0);
	this->prev_prev_spl = std::complex<R> ((R)0.0, (R)0.0);

	this->curr_idx           = 8369;
	this->loop_filter_state  = (R)0.0;
	this->integ_filter_state = (R)0.0;
	this->DDS_prev_in        = (R)0.0;
	this->is_active          = false;
	this->estimated_freq     = (R)0.0;
	this->mult.reset_time();
	this->mult.set_nu((R)0.0);
}


// ==================================================================================== explicit template instantiation
template class aff3ct::module::Synchronizer_coarse_fr_cc_DVBS2O<float>;
template class aff3ct::module::Synchronizer_coarse_fr_cc_DVBS2O<double>;
// ==================================================================================== explicit template instantiation

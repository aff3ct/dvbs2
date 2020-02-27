#include <cassert>
#include <iostream>

#include "Synchronizer_freq_coarse_DVBS2_aib.hpp"

// _USE_MATH_DEFINES does not seem to work on MSVC...
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923132169163975144
#endif

using namespace aff3ct::module;

template <typename R>
Synchronizer_freq_coarse_DVBS2_aib<R>
::Synchronizer_freq_coarse_DVBS2_aib(const int N, const int samples_per_symbol, const R damping_factor, const R normalized_bandwidth, const int n_frames)
:Synchronizer_freq_coarse<R>(N, n_frames), scrambled_pilots(this->PL_RAND_SEQ.size(),std::complex<R>((R)0,(R)0)), samples_per_symbol(samples_per_symbol), length_max(N/(2*samples_per_symbol)), proportional_gain((R)1.0), integrator_gain((R)1.0), digital_synthesizer_gain((R)1), prev_spl(std::complex<R>((R)0.0,(R)0.0)), prev_prev_spl(std::complex<R>((R)0.0,(R)0.0)), loop_filter_state((R)0.0),  integ_filter_state((R)0.0), DDS_prev_in((R)0.0), mult(N,(R)0.0, (R)1.0, 1)
{
	this->curr_idx = this->length_max - 1;
	this->set_PLL_coeffs (1, damping_factor, normalized_bandwidth);
	// std::cerr << "# PLL integrator_gain          = " << this->integrator_gain << std::endl;
	// std::cerr << "# PLL digital_synthesizer_gain = " << this->digital_synthesizer_gain << std::endl;
	// std::cerr << "# PLL proportional_gain        = " << this->proportional_gain << std::endl;
	// std::cerr << "# PL rand seq size             = " << this->PL_RAND_SEQ.size() << std::endl;
	for (size_t i = 0; i < this->PL_RAND_SEQ.size(); i++)
	{
		this->scrambled_pilots[i] = std::complex<R>(std::cos((R)M_PI_2*((R)PL_RAND_SEQ[i]+0.5)), std::sin((R)M_PI_2*((R)PL_RAND_SEQ[i]+0.5)));
	}
}

template <typename R>
Synchronizer_freq_coarse_DVBS2_aib<R>
::~Synchronizer_freq_coarse_DVBS2_aib()
{}

template <typename R>
void Synchronizer_freq_coarse_DVBS2_aib<R>
::_synchronize(const R *X_N1, R *Y_N2, const int frame_id)
{
	//for (int i = 1; i<this->N_in; i++)
	//	Y_N2[i] = X_N1[i];
	this->mult.imultiply(X_N1, Y_N2, frame_id);
}

template <typename R>
void Synchronizer_freq_coarse_DVBS2_aib<R>
::step(const std::complex<R>* x_elt, std::complex<R>* y_elt)
{
	//*y_elt = *x_elt;
	this->mult.step(x_elt, y_elt);
}


template <typename R>
void Synchronizer_freq_coarse_DVBS2_aib<R>
::update_phase(const std::complex<R> spl)
{
	//std::cerr << std::endl << this->curr_idx << std::endl;
	int rem_pos = this->curr_idx % 1476;
	if (rem_pos >= 54 && rem_pos < 90 && this->curr_idx >= 1530)//1530 54
	{
		int prev_prev_idx = (this->curr_idx - 2	)% this->length_max;

		R phase_error = std::imag( spl                              * this->scrambled_pilots[prev_prev_idx ]
									* std::conj( this->prev_prev_spl * this->scrambled_pilots[this->curr_idx] )
									);
		//std::cerr << std::endl << "idx " << this->curr_idx << " | spl " << spl << " | scrambled_pilots[this->curr_idx] " << this->scrambled_pilots[this->curr_idx] << " | scrambled_pilots[prev_prev_idx ] where prev_prev_idx = " << prev_prev_idx << " is " << this->scrambled_pilots[prev_prev_idx ] << " | prev_prev_spl " << this->prev_prev_spl << std::endl;
		// std::cerr << std::endl << "Phase error : "<< phase_error << std::endl;
		this->loop_filter_state += phase_error * this->integrator_gain;

		this->integ_filter_state += this->DDS_prev_in ;

		this->DDS_prev_in = phase_error * this->proportional_gain + this->loop_filter_state;

		this->estimated_freq = this->digital_synthesizer_gain * this->integ_filter_state / (R)this->samples_per_symbol;
		this->mult.set_nu(-this->estimated_freq);//

		this->prev_prev_spl = this->prev_spl;
		this->prev_spl = spl;

		//std::cerr << "curr_idx = " << this->curr_idx << " | phase_error = " << phase_error << " | loop_filter_state = " << this->loop_filter_state << " | integ_filter_state = " << this->integ_filter_state << " | nu = "<< -this->digital_synthesizer_gain * this->integ_filter_state / this->samples_per_symbol << std::endl;
	}
	else if (rem_pos == 90 && this->curr_idx >= 1530) // 1530
	{
		this->prev_prev_spl = std::complex<R>((R)0.0, (R)0.0);
		this->prev_spl      = std::complex<R>((R)0.0, (R)0.0);
	}
	this->curr_idx = (this->curr_idx + 1)% this->length_max;
}

template <typename R>
void Synchronizer_freq_coarse_DVBS2_aib<R>
::set_PLL_coeffs (const int pll_sps, const R damping_factor, const R normalized_bandwidth)
{
	R phase_error_detector_gain = (R)2.0;
	R phase_recovery_loop_bandwidth = normalized_bandwidth * (R)pll_sps;

	//K0
	R phase_recovery_gain = (R)pll_sps;

	R theta = phase_recovery_loop_bandwidth/((damping_factor + 0.25/damping_factor)*(R)pll_sps);
	R d = (R)1.0 + (R)2.0*damping_factor*theta + theta*theta;

	//K1
	this->proportional_gain = ((R)4.0*damping_factor*theta/d)/(phase_error_detector_gain*phase_recovery_gain);

	//K2
	this->integrator_gain = ((R)4.0/(R)pll_sps*theta*theta/d)/(phase_error_detector_gain*phase_recovery_gain);
	this->digital_synthesizer_gain = (R)1.0;
}

template <typename R>
void Synchronizer_freq_coarse_DVBS2_aib<R>
::_reset()
{
	this->prev_spl      = std::complex<R> ((R)0.0, (R)0.0);
	this->prev_prev_spl = std::complex<R> ((R)0.0, (R)0.0);

	this->curr_idx           = this->length_max - 1;
	this->loop_filter_state  = (R)0.0;
	this->integ_filter_state = (R)0.0;
	this->DDS_prev_in        = (R)0.0;

	this->mult.reset();
	this->mult.set_nu((R)0.0);
}


// ==================================================================================== explicit template instantiation
template class aff3ct::module::Synchronizer_freq_coarse_DVBS2_aib<float>;
template class aff3ct::module::Synchronizer_freq_coarse_DVBS2_aib<double>;
// ==================================================================================== explicit template instantiation

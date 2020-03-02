#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_Gardner_fast_osf2.hpp"

namespace aff3ct
{
namespace module
{

template <typename B, typename R>
void Synchronizer_Gardner_fast_osf2<B,R>
::step(const std::complex<R> *X_N1, std::complex<R> *Y_N1, B *B_N1)
{
	farrow_flt.step( X_N1, Y_N1);
	B_N1[0] = this->is_strobe;
	B_N1[1] = this->is_strobe;

	this->last_symbol = (this->is_strobe == 1)?*Y_N1:this->last_symbol;

	this->TED_update(*Y_N1);
	this->loop_filter();
	this->interpolation_control();
}

template <typename B, typename R>
void Synchronizer_Gardner_fast_osf2<B,R>
::loop_filter()
{
	//this->lf_filter_state += this->lf_prev_in;
	//this->lf_output        = this->TED_error * this->lf_proportional_gain + this->lf_filter_state;
	//this->lf_prev_in       = this->TED_error * this->lf_integrator_gain;

	R vp = this->TED_error * this->lf_proportional_gain;
	R vi = this->lf_prev_in + this->TED_error * this->lf_integrator_gain;
	this->lf_prev_in = vi;
	this->lf_output = vp + vi;
}

template <typename B, typename R>
void Synchronizer_Gardner_fast_osf2<B,R>
::interpolation_control()
{
	// Interpolation Control
	R W = this->lf_output + (R)0.5;
	this->prev_is_strobe = this->is_strobe;
	this->is_strobe = (this->NCO_counter < W) ? 1:0; // Check if a strobe
	if (this->is_strobe == 1) // Update mu if a strobe
	{
		this->mu = this->NCO_counter / W;
		this->farrow_flt.set_mu(this->mu);
		this->NCO_counter += (R)1.0f;
	}

	this->NCO_counter = (this->NCO_counter - W); // Update counter*/
}

template <typename B, typename R>
void Synchronizer_Gardner_fast_osf2<B,R>
::TED_update(std::complex<R> sample)
{
	R* TED_buffer_iq    = reinterpret_cast<R* >(this->TED_buffer.data());
	R* sample_iq        = reinterpret_cast<R* >(&sample);

	int strobe_history = this->is_strobe + this->prev_is_strobe * 2;
	if (strobe_history == 1)
		this->TED_error = TED_buffer_iq[2] * (TED_buffer_iq[0] - sample_iq[0]) +
			              TED_buffer_iq[3] * (TED_buffer_iq[1] - sample_iq[1]);
	else
		this->TED_error = 0.0f;

	// Stuffing / skipping
	switch (strobe_history)
	{
		case 1:
			this->TED_buffer[0].real((R)0.0f);
			this->TED_buffer[0].imag((R)0.0f);

			this->TED_buffer[1] = sample;
		break;

		case 0:
		break;

		default:
			this->TED_buffer[0] = this->TED_buffer[1];
			this->TED_buffer[1] = sample;
		break;
	}
}

}
}

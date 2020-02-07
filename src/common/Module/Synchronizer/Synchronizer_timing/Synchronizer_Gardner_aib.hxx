#include "aff3ct.hpp"
#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_Gardner_aib.hpp"

namespace aff3ct
{
namespace module
{

template <typename B, typename R>
void Synchronizer_Gardner_aib<B,R>
::step(const std::complex<R> *X_N1, std::complex<R>* Y_N1, B* B_N1)
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
void Synchronizer_Gardner_aib<B,R>
::TED_update(std::complex<R> sample)
{
	this->strobe_history = (this->strobe_history << 1) % this->POW_osf + this->is_strobe;
	if (this->strobe_history == 1)
	{
		this->TED_error = std::real(this->TED_buffer[this->TED_mid_pos]) * (std::real(this->TED_buffer[this->TED_head_pos]) - std::real(sample)) +
		                  std::imag(this->TED_buffer[this->TED_mid_pos]) * (std::imag(this->TED_buffer[this->TED_head_pos]) - std::imag(sample));
	}
	else
		this->TED_error = 0.0f;

	// Stuffing / skipping
	switch (this->set_bits_nbr[this->strobe_history])
	{
		case 0:
		break;

		case 1:
			this->TED_buffer[this->TED_head_pos] = sample;

			this->TED_head_pos = (this->TED_head_pos - 1 + this->osf) % this->osf;
			this->TED_mid_pos  = (this->TED_mid_pos  - 1 + this->osf) % this->osf;
		break;

		default:
			this->TED_buffer[ this->TED_head_pos               ] = std::complex<R>(0.0f, 0.0f);
			this->TED_buffer[(this->TED_head_pos - 1 + this->osf)%this->osf] = sample;

			this->TED_head_pos     = (this->TED_head_pos - 2 + this->osf) % this->osf;
			this->TED_mid_pos      = (this->TED_mid_pos  - 2 + this->osf) % this->osf;
		break;
	}
}

template <typename B, typename R>
void Synchronizer_Gardner_aib<B,R>
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
void Synchronizer_Gardner_aib<B, R>
::interpolation_control()
{
	// Interpolation Control
	R W = this->lf_output + this->INV_osf;
	this->is_strobe = (this->NCO_counter < W) ? 1:0; // Check if a strobe
	if (this->is_strobe == 1) // Update mu if a strobe
	{
		this->mu = this->NCO_counter / W;
		this->farrow_flt.set_mu(this->mu);
		this->NCO_counter += (R)1.0f;
	}

	this->NCO_counter = (this->NCO_counter - W); // Update counter*/

	//this->is_strobe = ((int)this->NCO_counter % 4 == 0) ? 1:0; // Check if a strobe
	//this->NCO_counter += 1.0f;
	//this->NCO_counter = (R)((int)this->NCO_counter % 4);
}

}
}
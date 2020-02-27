#include "aff3ct.hpp"
#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_Gardner_ultra_osf2.hpp"

namespace aff3ct
{
namespace module
{

template <typename B, typename R>
void Synchronizer_Gardner_ultra_osf2<B,R>
::step(const std::complex<R> *X_N1, std::complex<R>* Y_N1, B* B_N1)
{
	farrow_flt.step( X_N1, Y_N1);
	B_N1[0] = this->is_strobe;
	B_N1[1] = this->is_strobe;

	this->last_symbol = (this->is_strobe == 1)?*Y_N1:this->last_symbol;

	this->TED_update(*Y_N1);
	this->loop_filter();
	this->interpolation_control(this->lf_output, this->NCO_counter, this->mu, this->is_strobe);
	if (this->is_strobe)
		this->farrow_flt.set_mu(this->mu);
}

/*template <typename B, typename R>
void Synchronizer_Gardner_ultra_osf2<B,R>
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

			this->TED_head_pos = this->TED_mid_pos;
			this->TED_mid_pos  = 1-this->TED_mid_pos;
		break;

		default:
			this->TED_buffer[ this->TED_head_pos ] = std::complex<R>(0.0f, 0.0f);
			this->TED_buffer[1-this->TED_head_pos] = sample;
		break;
	}
}*/
template <typename B, typename R>
void Synchronizer_Gardner_ultra_osf2<B,R>
::TED_update(std::complex<R> sample)
{
	auto old_is_strobe   = this->strobe_history % 2;
	this->strobe_history = old_is_strobe * 2 + this->is_strobe;
	R* TED_buffer_iq    = reinterpret_cast<R* >(this->TED_buffer.data());
	R* sample_iq        = reinterpret_cast<R* >(&sample);
	if (this->strobe_history == 1)
	{
		this->TED_error = TED_buffer_iq[2] * (TED_buffer_iq[0] - sample_iq[0]) +
			              TED_buffer_iq[3] * (TED_buffer_iq[1] - sample_iq[1]);
	}
	else
		this->TED_error = 0.0f;

	// Stuffing / skipping
	if ((old_is_strobe ^ this->is_strobe) == 1)
	{
			this->TED_buffer[0] = this->TED_buffer[1];
			this->TED_buffer[1] = sample;
	}
	else if((old_is_strobe & this->is_strobe) ==1)
	{
			TED_buffer_iq[0] = 0;
			TED_buffer_iq[1] = 0;

			this->TED_buffer[1] = sample;
	}
}

template <typename B, typename R>
void Synchronizer_Gardner_ultra_osf2<B,R>
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
void Synchronizer_Gardner_ultra_osf2<B, R>
::interpolation_control(R lf_output, R &NCO_counter, R& mu, int &is_strobe)
{
	// Interpolation Control
	R W = lf_output + 0.5f;
	is_strobe = (NCO_counter < W) ? 1:0; // Check if a strobe
	if (is_strobe == 1) // Update mu if a strobe
	{
		mu = NCO_counter / W;
		NCO_counter += 1.0f;
	//	this->farrow_flt.set_mu(this->mu);
	}
	NCO_counter = (NCO_counter - W) ; // Update counter

	//this->is_strobe = ((int)this->NCO_counter % 4 == 0) ? 1:0; // Check if a strobe
	//this->NCO_counter += 1.0f;
	//this->NCO_counter = (R)((int)this->NCO_counter % 4);
}

template <typename B, typename R>
R Synchronizer_Gardner_ultra_osf2<B, R>
::compute_mu(R NCO_counter, R W)
{
	return NCO_counter / W - std::floor(NCO_counter / W);
}
}
}
#include <cassert>
#include <iostream>
#include <vector>
#include <complex>

#include "Synchronizer_Gardner_cc_naive.hpp"

// _USE_MATH_DEFINES does not seem to work on MSVC...
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

using namespace aff3ct::module;

template <typename R>
Synchronizer_Gardner_cc_naive<R>
::Synchronizer_Gardner_cc_naive(const int N, int OSF, const R damping_factor, const R normalized_bandwidth, const R detector_gain)
: Synchronizer<R>(N,N/OSF),
OSF(OSF),
POW_OSF(1<<OSF),
INV_OSF((R)1.0/ (R)OSF),
last_symbol(0,0),
mu(0),
farrow_flt(N,(R)0),
strobe_history(0),
is_strobe(0),
TED_error((R)0),
TED_buffer(OSF, std::complex<R>((R)0,(R)0)),
TED_head_pos(OSF - 1),
TED_mid_pos((OSF - 1 - OSF / 2) % OSF),
lf_proportional_gain((R)0),
lf_integrator_gain   ((R)0),
lf_prev_in ((R)0),
lf_filter_state ((R)0),
lf_output((R)0),
NCO_counter((R)0),
overflow_cnt(0),
underflow_cnt(0),
output_buffer(N/OSF, std::complex<R>((R)0,(R)0)),
outbuf_head  (0),//outbuf_head  (0),//N/OSF/10
outbuf_max_sz(N/OSF),
outbuf_cur_sz(0)//outbuf_cur_sz(0)//N/OSF/10
{
	this->set_loop_filter_coeffs(damping_factor, normalized_bandwidth, detector_gain);
}

template <typename R>
Synchronizer_Gardner_cc_naive<R>
::~Synchronizer_Gardner_cc_naive()
{}

template <typename R>
void Synchronizer_Gardner_cc_naive<R>
::_synchronize(const R *X_N1, R *Y_N2, const int frame_id)
{
	auto cX_N1 = reinterpret_cast<const std::complex<R>* >(X_N1);
	auto cY_N2 = reinterpret_cast<      std::complex<R>* >(Y_N2);

	for (auto i = 0; i < this->N_in/2 ; i++)
		this->step(&cX_N1[i]);

	for (auto i = 0; i < this->N_out/2; i++)
		this->pop(&cY_N2[i]);
}


template <typename R>
void Synchronizer_Gardner_cc_naive<R>
::step(const std::complex<R> *X_N1)
{
	std::complex<R> farrow_output(0,0);
	farrow_flt.step( X_N1, &farrow_output);
	if (this->is_strobe)
	{
		this->push(farrow_output);
		this->last_symbol = farrow_output;
	}

	this->TED_update(farrow_output);
	this->loop_filter();
	this->interpolation_control();
}

template <typename R>
void Synchronizer_Gardner_cc_naive<R>
::reset()
{
	this->farrow_flt.reset();
	this->farrow_flt.set_mu((R)0);

	for (auto i = 0; i<this->OSF ; i++)
		this->TED_buffer[i] = std::complex<R>(R(0),R(0));

	this->last_symbol      = std::complex<R> (R(0),R(0));
	this->mu               = (R)0;
	this->strobe_history   = 0;
	this->is_strobe        = 0;
	this->TED_error        = (R)0;
	this->TED_head_pos     = 0;
	this->TED_mid_pos      = OSF/2;//(OSF - 1 - OSF / 2) % OSF;
	this->lf_prev_in       = (R)0;
	this->lf_filter_state  = (R)0;
	this->lf_output        = (R)0;
	this->NCO_counter      = (R)0;

	this->overflow_cnt     = 0;
	this->underflow_cnt    = 0;
	for (auto i = 0; i<this->outbuf_max_sz ; i++)
		this->output_buffer[i] = std::complex<R>((R)0,(R)0);

	this->outbuf_head      = 0; // this->N_out/10;
	this->outbuf_tail      = 0;
	this->outbuf_cur_sz    = 0; //this->N_out/10;
}

template <typename R>
void Synchronizer_Gardner_cc_naive<R>
::loop_filter()
{
	this->lf_filter_state += this->lf_prev_in;
	this->lf_output        = this->TED_error * this->lf_proportional_gain + this->lf_filter_state;
	this->lf_prev_in       = this->TED_error * this->lf_integrator_gain;
	
	//R vp = this->TED_error * this->lf_proportional_gain;
	//R vi = this->lf_prev_in + this->TED_error * this->lf_integrator_gain;
	//this->lf_prev_in = vi;
	//this->lf_output = vp + vi;
}

template <typename R>
void Synchronizer_Gardner_cc_naive<R>
::interpolation_control()
{
	// Interpolation Control
	R W = this->lf_output + this->INV_OSF;
	this->is_strobe = (this->NCO_counter < W) ? 1:0; // Check if a strobe
	if (this->is_strobe == 1) // Update mu if a strobe
	{
		this->mu = this->NCO_counter / W;
		this->farrow_flt.set_mu(this->mu);
	}

	this->NCO_counter = (this->NCO_counter - W) - std::floor(this->NCO_counter - W); // Update counter*/
}

template <typename R>
void Synchronizer_Gardner_cc_naive<R>
::TED_update(std::complex<R> sample)
{
	this->strobe_history = (this->strobe_history << 1) % this->POW_OSF + this->is_strobe;

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

			this->TED_head_pos = (this->TED_head_pos + this->OSF - 1) % this->OSF;
			this->TED_mid_pos  = (this->TED_mid_pos  + this->OSF - 1) % this->OSF;
		break;

		default:
			this->TED_buffer[ this->TED_head_pos               ] = std::complex<R>(0.0f, 0.0f);
			this->TED_buffer[(this->TED_head_pos - 1)%this->OSF] = sample;

			this->TED_head_pos     = (this->TED_head_pos + this->OSF - 2) % this->OSF;
			this->TED_mid_pos      = (this->TED_mid_pos  + this->OSF - 2) % this->OSF;
		break;
	}
}

template <typename R>
void Synchronizer_Gardner_cc_naive<R>
::push(const std::complex<R> strobe)
{
	if (this->outbuf_cur_sz < this->outbuf_max_sz)
	{
		this->output_buffer[this->outbuf_head] = strobe;
		this->outbuf_head = (this->outbuf_head + 1)%this->outbuf_max_sz;
		this->outbuf_cur_sz++;
	}
	else
	{
		this->overflow_cnt++;
	}
}

template <typename R>
void Synchronizer_Gardner_cc_naive<R>
::pop(std::complex<R> *strobe)
{
	if	(this->outbuf_cur_sz > 0)
	{
		*strobe = this->output_buffer[this->outbuf_tail];
		this->outbuf_tail = (this->outbuf_tail + 1)%this->outbuf_max_sz;
		this->outbuf_cur_sz--;
	}
	else
	{
		*strobe = std::complex<R>((R)0,(R)0);
		this->underflow_cnt++;
	}
}

template <typename R>
int Synchronizer_Gardner_cc_naive<R>
::get_delay()
{
	return 0;//this->outbuf_cur_sz;//
}

template <typename R>
void Synchronizer_Gardner_cc_naive<R>
::set_loop_filter_coeffs (const R damping_factor, const R normalized_bandwidth, const R detector_gain)
{
	R K0   = -1;
	R theta = normalized_bandwidth/this->OSF/(damping_factor + 0.25/damping_factor);      
	R d  = (1 + 2*damping_factor*theta + theta*theta) * K0 * detector_gain;
	
	this->lf_proportional_gain = (4*damping_factor*theta) /d;
	this->lf_integrator_gain   = (4*theta*theta)/d;
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Synchronizer_Gardner_cc_naive<float>;
template class aff3ct::module::Synchronizer_Gardner_cc_naive<double>;
// ==================================================================================== explicit template instantiation

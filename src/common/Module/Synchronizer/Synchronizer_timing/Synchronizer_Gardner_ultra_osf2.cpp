#include <cassert>
#include <iostream>

#include "aff3ct.hpp"
#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_Gardner_ultra_osf2.hpp"

using namespace aff3ct::module;

template <typename B, typename R>
Synchronizer_Gardner_ultra_osf2<B, R>
::Synchronizer_Gardner_ultra_osf2(const int N, int hold_size, const R damping_factor, const R normalized_bandwidth, const R detector_gain, const int n_frames)
: Synchronizer_timing<B,R>(N, 2, n_frames),
farrow_flt(2*hold_size-8,(R)0),
//farrow_flt(N,(R)0),
strobe_history(0),
TED_error((R)0),
TED_buffer(2, std::complex<R>((R)0,(R)0)),
hold_size(hold_size),
lf_proportional_gain((R)0),
lf_integrator_gain   ((R)0),
lf_prev_in ((R)0),
lf_filter_state ((R)0),
lf_output((R)0),
NCO_counter((R)0),
buffer_mtx()
{
	assert(hold_size > 4);
	this->set_loop_filter_coeffs(damping_factor, normalized_bandwidth, detector_gain);
}

template <typename B, typename R>
Synchronizer_Gardner_ultra_osf2<B, R>
::~Synchronizer_Gardner_ultra_osf2()
{
}

/*template <typename B, typename R>
void Synchronizer_Gardner_ultra_osf2<B, R>
::_synchronize(const R *X_N1, R *Y_N1, B *B_N1, const int frame_id)
{
	auto cX_N1 = reinterpret_cast<const std::complex<R>*>(X_N1);
	auto cY_N1 = reinterpret_cast<      std::complex<R>*>(Y_N1);

	for (auto i = 0; i < this->N_in/2 ; i+= 1)
	{
		farrow_flt.step( cX_N1 + i, cY_N1 + i);
		this->TED_update(cY_N1[i]);

		B_N1[2*i + 0] = this->is_strobe;
		B_N1[2*i + 1] = this->is_strobe;

		this->loop_filter();
		this->interpolation_control();
	}
}
*/


template <typename B, typename R>
void Synchronizer_Gardner_ultra_osf2<B, R>
::_synchronize(const R *X_N1, R *Y_N1, B *B_N1, const int frame_id)
{
	auto cX_N1 = reinterpret_cast<const std::complex<R>*>(X_N1);
	auto cY_N1 = reinterpret_cast<      std::complex<R>*>(Y_N1);
	//std::vector<R> v_mu (this->N_in/2, 0);
	//std::vector<int> v_strobe_history (this->N_in/2, 0);
	//std::vector<int> v_is_strobe (this->N_in/2, 0);
	//std::vector<R> v_NCO_counter (this->N_in/2, 0);
	//std::vector<R> v_TED_error (this->N_in/2, 0);
	//std::vector<R> v_lf_output (this->N_in/2, 0);

	if (this->act)
	{
		int hold_nbr = (this->N_in/2) / this->hold_size;
		int tail_nbr = this->N_in/2 - hold_nbr * this->hold_size;

		for(int i = 0; i < hold_nbr * this->hold_size; i+=this->hold_size)
		{
			farrow_flt.filter( X_N1 + 2*i, Y_N1 + 2*i);
			int p = this->is_strobe;
			for (int j = 0; j<this->hold_size-4; j++)
			{
				B_N1[2*(i + j) + 0] = p;
				B_N1[2*(i + j) + 1] = p;
				p = 1-p;
			}
			for (int j = 0; j<this->hold_size-4; j++)
			{
				this->TED_update(cY_N1[i+j]);
				this->loop_filter();
				this->is_strobe   = 1-this->is_strobe; // Check if a strobe
				this->NCO_counter += (R)this->is_strobe - 0.5;
			}

			for(int j = this->hold_size-4; j<this->hold_size; j++)
			{
				farrow_flt.step( cX_N1 + i + j, cY_N1 + i + j);
				B_N1[2*(i+j) + 0] = this->is_strobe;
				B_N1[2*(i+j) + 1] = this->is_strobe;

				this->TED_update(cY_N1[i+j]);
				this->loop_filter();
				this->interpolation_control(this->lf_output, this->NCO_counter, this->mu, this->is_strobe);this->farrow_flt.set_mu(this->mu);
			}

			//v_mu            [i+this->hold_size-1] = this->mu;
			//v_strobe_history[i+this->hold_size-1] = this->strobe_history;
			//v_is_strobe     [i+this->hold_size-1] = this->is_strobe;
			//v_NCO_counter   [i+this->hold_size-1] = this->NCO_counter;
			//v_TED_error     [i+this->hold_size-1] = this->TED_error;
			//v_lf_output     [i+this->hold_size-1] = this->lf_output;

			//v_mu            [i+this->hold_size-1] = this->mu;
			//v_strobe_history[i+this->hold_size-1] = this->strobe_history;
			//v_is_strobe     [i+this->hold_size-1] = this->is_strobe;
			//v_NCO_counter   [i+this->hold_size-1] = this->NCO_counter;
			//v_TED_error     [i+this->hold_size-1] = this->TED_error;
			//v_lf_output     [i+this->hold_size-1] = this->lf_output;
		}

		int i = hold_nbr * this->hold_size;
		for (auto j = 0; j < tail_nbr ; j++)
		{
			farrow_flt.step( cX_N1 + i + j, cY_N1 + i + j);
			this->TED_update(cY_N1[i+j]);

			B_N1[2*(i+j) + 0] = this->is_strobe;
			B_N1[2*(i+j) + 1] = this->is_strobe;

			this->loop_filter();
			this->interpolation_control(this->lf_output, this->NCO_counter, this->mu, this->is_strobe);this->farrow_flt.set_mu(this->mu);
		}
	}
	else
	{
		for (auto i = 0; i < this->N_in/2 ; i+= 1)
		{
			//v_mu            [i] = this->mu;
			//v_strobe_history[i] = this->strobe_history;
			//v_is_strobe     [i] = this->is_strobe;
			//v_NCO_counter   [i] = this->NCO_counter;
			//v_TED_error     [i] = this->TED_error;
			//v_lf_output     [i] = this->lf_output;

			farrow_flt.step( cX_N1 + i, cY_N1 + i);
			this->TED_update(cY_N1[i]);

			B_N1[2*i + 0] = this->is_strobe;
			B_N1[2*i + 1] = this->is_strobe;

			this->loop_filter();
			this->interpolation_control(this->lf_output, this->NCO_counter, this->mu, this->is_strobe);this->farrow_flt.set_mu(this->mu);
		}
	}
	/*if((*this)[stm::tsk::synchronize].is_debug())
	{
		std::cout << "# {INTERNAL} mu = [";
		for (auto i = 0; i < this->N_in/2 ; i++)
			std::cout << v_mu[i] << " ";
		std::cout << " ]" << std::endl;

		std::cout << "# {INTERNAL} is_strobe = [";
		for (auto i = 0; i < this->N_in/2 ; i++)
			std::cout << v_is_strobe[i] << " ";
		std::cout << " ]" << std::endl;

		std::cout << "# {INTERNAL} strobe_history = [";
		for (auto i = 0; i < this->N_in/2 ; i++)
			std::cout << v_strobe_history[i] << " ";
		std::cout << " ]" << std::endl;

		std::cout << "# {INTERNAL} NCO_counter = [";
		for (auto i = 0; i < this->N_in/2 ; i++)
			std::cout << v_NCO_counter[i] << " ";
		std::cout << " ]" << std::endl;

		std::cout << "# {INTERNAL} TED_error = [";
		for (auto i = 0; i < this->N_in/2 ; i++)
			std::cout << v_TED_error[i] << " ";
		std::cout << " ]" << std::endl;

		std::cout << "# {INTERNAL} lf_output = [";
		for (auto i = 0; i < this->N_in/2 ; i++)
			std::cout << v_lf_output[i] << " ";
		std::cout << " ]" << std::endl;
	}*/
}



/*template <typename B, typename R>
void Synchronizer_Gardner_ultra_osf2<B, R>
::_synchronize(const R *X_N1, R *Y_N1, B *B_N1, const int frame_id)
{
	auto cX_N1 = reinterpret_cast<const std::complex<R>*>(X_N1);
	auto cY_N1 = reinterpret_cast<      std::complex<R>*>(Y_N1);
	std::vector<R> v_mu (this->N_in/2, 0);
	std::vector<int> v_strobe_history (this->N_in/2, 0);
	std::vector<int> v_is_strobe (this->N_in/2, 0);
	std::vector<R> v_NCO_counter (this->N_in/2, 0);
	std::vector<R> v_TED_error (this->N_in/2, 0);
	std::vector<R> v_lf_output (this->N_in/2, 0);

	for (auto i = 0; i < this->N_in/2 ; i++)
	{
		v_mu[i] = this->mu;
		v_strobe_history[i] = this->strobe_history;
		v_is_strobe[i] = this->is_strobe;
		v_NCO_counter[i] = this->NCO_counter;
		v_TED_error[i] = this->TED_error;
		v_lf_output[i] = this->lf_output;
		this->step(&cX_N1[i], &cY_N1[i], &B_N1[2*i]);
	}
	if((*this)[stm::tsk::synchronize].is_debug())
	{
		std::cout << "# {INTERNAL} mu = [";
		for (auto i = 0; i < this->N_in/2 ; i++)
			std::cout << v_mu[i] << " ";
		std::cout << " ]" << std::endl;

		std::cout << "# {INTERNAL} is_strobe = [";
		for (auto i = 0; i < this->N_in/2 ; i++)
			std::cout << v_is_strobe[i] << " ";
		std::cout << " ]" << std::endl;

		std::cout << "# {INTERNAL} strobe_history = [";
		for (auto i = 0; i < this->N_in/2 ; i++)
			std::cout << v_strobe_history[i] << " ";
		std::cout << " ]" << std::endl;

		std::cout << "# {INTERNAL} NCO_counter = [";
		for (auto i = 0; i < this->N_in/2 ; i++)
			std::cout << v_NCO_counter[i] << " ";
		std::cout << " ]" << std::endl;

		std::cout << "# {INTERNAL} TED_error = [";
		for (auto i = 0; i < this->N_in/2 ; i++)
			std::cout << v_TED_error[i] << " ";
		std::cout << " ]" << std::endl;

		std::cout << "# {INTERNAL} lf_output = [";
		for (auto i = 0; i < this->N_in/2 ; i++)
			std::cout << v_lf_output[i] << " ";
		std::cout << " ]" << std::endl;
	}
}*/

/*template <typename B, typename R>
void Synchronizer_Gardner_ultra_osf2<B, R>
::_synchronize(const R *X_N1, R *Y_N1, B *B_N1, const int frame_id)
{
	auto cX_N1 = reinterpret_cast<const std::complex<R>*>(X_N1);
	auto cY_N1 = reinterpret_cast<      std::complex<R>*>(Y_N1);

	int m     = this->is_strobe == 1 ? 0 : 1;
	int old_m = 2 - this->strobe_history;

	if (this->TED_head_pos == 1)
	{
		auto temp = this->TED_buffer[0];
		this->TED_buffer[0] = this->TED_buffer[1];
		this->TED_buffer[1] = temp;
		this->TED_head_pos = 0;
		this->TED_mid_pos = 1;
	}

	for (auto i = 0; i < this->N_in/2 ; i+=2)
	{
		farrow_flt.step( &cX_N1[i+0], &cY_N1[i+0]);
		farrow_flt.step( &cX_N1[i+1], &cY_N1[i+1]);

		B_N1[2*(i+m)   + 0] = 1;
		B_N1[2*(i+m)   + 1] = 1;
		B_N1[2*(i+1-m) + 0] = 0;
		B_N1[2*(i+1-m) + 1] = 0;

		if(m == 0)
			this->TED_error = std::real(this->TED_buffer[1]) * (std::real(this->TED_buffer[0]) - std::real(cY_N1[i+0])) +
							  std::imag(this->TED_buffer[1]) * (std::imag(this->TED_buffer[0]) - std::imag(cY_N1[i+0]));
		else
			this->TED_error = std::real(cY_N1[i+0]) * (std::real(this->TED_buffer[1]) - std::real(cY_N1[i+1])) +
							  std::imag(cY_N1[i+0]) * (std::imag(this->TED_buffer[1]) - std::imag(cY_N1[i+1]));

		this->loop_filter();

		R new_mu = this->mu + 1/(this->lf_output + 0.5);
		R diff_m = std::floor(new_mu);
		old_m    = m;
		m        = (m + (int)diff_m) % 2;
		this->mu = new_mu - diff_m;
		this->farrow_flt.set_mu(this->mu);

		if(((int)diff_m == 3 && old_m == 1) || (((int)diff_m == 1 && old_m == 0)))
		{
			B_N1[2*(i+1) + 0] = 1-B_N1[2*(i+1) + 0];
			B_N1[2*(i+1) + 1] = 1-B_N1[2*(i+1) + 1];
			farrow_flt.step( &cX_N1[i+1], &cY_N1[i+1]);
		}

		if (old_m == 0 && m == 1)
		{
			this->TED_buffer[1] = cY_N1[i+1];
		}
		else if (old_m == 1 && m == 0)
		{
			this->TED_buffer[0] = cY_N1[i+1];
			this->TED_buffer[1].real(0);
			this->TED_buffer[1].imag(0);
		}
		else
		{
			this->TED_buffer[0] = cY_N1[i+0];
			this->TED_buffer[1] = cY_N1[i+1];
		}
	}

	this->is_strobe = 1-m;
	this->strobe_history = 2 - old_m;
}
*/

template <typename B, typename R>
void Synchronizer_Gardner_ultra_osf2<B, R>
::_reset()
{
	this->mu = (R)0;
	this->farrow_flt.reset();
	this->farrow_flt.set_mu((R)0);

	for (auto i = 0; i<this->osf ; i++)
		this->TED_buffer[i] = std::complex<R>(R(0),R(0));

	this->strobe_history  = 0;
	this->TED_error       = (R)0;
	this->lf_prev_in      = (R)0;
	this->lf_filter_state = (R)0;
	this->lf_output       = (R)0;
	this->NCO_counter     = (R)0;
}

template <typename B, typename R>
void Synchronizer_Gardner_ultra_osf2<B, R>
::set_loop_filter_coeffs(const R damping_factor, const R normalized_bandwidth, const R detector_gain)
{
	R K0   = -1;
	R theta = normalized_bandwidth/(R)this->osf/(damping_factor + 0.25/damping_factor);
	R d  = (1 + 2*damping_factor*theta + theta*theta) * K0 * detector_gain;

	this->lf_proportional_gain = (4*damping_factor*theta) /d;
	this->lf_integrator_gain   = (4*theta*theta)/d;
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Synchronizer_Gardner_ultra_osf2<int, float>;
template class aff3ct::module::Synchronizer_Gardner_ultra_osf2<int, double>;
// ==================================================================================== explicit template instantiation

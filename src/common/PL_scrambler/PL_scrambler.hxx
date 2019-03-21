/*!
 * \file
 * \brief Generates a message.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef PL_SCRAMBLER_HXX_
#define PL_SCRAMBLER_HXX_

#include <sstream>
#include <complex>

#include "Tools/Exception/exception.hpp"

#include "PL_scrambler.hpp"

namespace aff3ct
{
namespace module
{

template <typename B>
PL_scrambler<B>::
PL_scrambler(const int FRAME_SIZE, const int start_ix, const bool scr_flag, const int n_frames)
: Module(n_frames), FRAME_SIZE(FRAME_SIZE), start_ix(start_ix), scr_flag(scr_flag)
{
	const std::string name = "PL_scrambler";
	this->set_name(name);
	this->set_short_name(name);

	if (FRAME_SIZE <= 0)
	{
		std::stringstream message;
		message << "'FRAME_SIZE' has to be greater than 0 ('FRAME_SIZE' = " << FRAME_SIZE << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}
	
	auto &p = this->create_task("scramble");
	auto &ps_clear_frame = this->template create_socket_out<B>(p, "clear_frame", this->FRAME_SIZE * this->n_frames);
	auto &ps_scrambled_frame = this->template create_socket_out<B>(p, "scrambled_frame", this->FRAME_SIZE * this->n_frames);
	this->create_codelet(p, [this, &ps_scrambled_frame, &ps_clear_frame]() -> int
	{
		this->scramble(static_cast<B*>(ps_scrambled_frame.get_dataptr()), static_cast<B*>(ps_clear_frame.get_dataptr()));

		return 0;
	});
}

/*template <typename B>
int PL_scrambler<B>::
get_K() const
{
	return K;
}*/

template <typename B>
template <class A>
void PL_scrambler<B>::
scramble(std::vector<B,A>& clear_frame, std::vector<B,A>& scrambled_frame, const int frame_id)
{
	if (this->FRAME_SIZE * this->n_frames != (int)clear_frame.size())
	{
		std::stringstream message;
		message << "'clear_frame.size()' has to be equal to 'FRAME_SIZE' * 'n_frames' ('clear_frame.size()' = " << clear_frame.size()
		        << ", 'FRAME_SIZE' = " << this->FRAME_SIZE << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}
	if (this->FRAME_SIZE * this->n_frames != (int)scrambled_frame.size())
	{
		std::stringstream message;
		message << "'scrambled_frame.size()' has to be equal to 'FRAME_SIZE' * 'n_frames' ('scrambled_frame.size()' = " << scrambled_frame.size()
		        << ", 'FRAME_SIZE' = " << this->FRAME_SIZE << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	this->scramble(clear_frame.data(), scrambled_frame.data(), frame_id);
}

template <typename B>
void PL_scrambler<B>::
scramble(B *clear_frame, B *scrambled_frame, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_scramble(clear_frame + f * this->FRAME_SIZE, scrambled_frame + f * this->FRAME_SIZE, f);
}
template <typename B>
void PL_scrambler<B>::
_scramble(B *clear_frame, B *scrambled_frame, const int frame_id)
{

		for(int i = this->start_ix; i < FRAME_SIZE/2; i++)
		{
			int R_lsb = this->PL_RAND_SEQ[i]%2;
			int R_msb = this->PL_RAND_SEQ[i]/2;
			int R_real = (1-R_lsb) * (-2*R_msb+1); // real part
			int R_imag = R_lsb*(-2*R_msb+1); // imag part
			R_imag = (2*scr_flag-1)*R_imag; // conjugate if scr_flag == false, i.e. descrambling
			float D_real = clear_frame[2*i];
			float D_imag = clear_frame[2*i+1];
			scrambled_frame[2*i] = R_real*D_real - R_imag*D_imag; // real part
			scrambled_frame[2*i + 1] = R_imag*D_real + R_real*D_imag; // imag part
		}
}

}
}

#endif

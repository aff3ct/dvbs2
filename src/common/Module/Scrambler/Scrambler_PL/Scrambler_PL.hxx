/*!
 * \file
 * \brief Generates a message.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef SCRAMBLER_PL_HXX_
#define SCRAMBLER_PL_HXX_

#include <sstream>
#include <complex>

#include "Tools/Exception/exception.hpp"

#include "Module/Scrambler/Scrambler_PL/Scrambler_PL.hpp"

namespace aff3ct
{
namespace module
{

template <typename D>
Scrambler_PL<D>::
Scrambler_PL(const int N, const int start_ix, const int n_frames)
: Scrambler<D>(N, n_frames), start_ix(start_ix)
{
	const std::string name = "Scrambler_PL";
	this->set_name(name);
	this->set_short_name(name);

}

template <typename D>
Scrambler_PL<D>* Scrambler_PL<D>
::clone() const
{
	auto m = new Scrambler_PL(*this);
	m->deep_copy(*this);
	return m;
}

template <typename D>
void Scrambler_PL<D>::
_scramble(D *X_N1, D *X_N2, const int frame_id)
{
	this->__scramble(X_N1, X_N2, true, frame_id);
}


template <typename D>
void Scrambler_PL<D>::
_descramble(D *Y_N1, D *Y_N2, const int frame_id)
{
	this->__scramble(Y_N1, Y_N2, false, frame_id);
}

template <typename D>
void Scrambler_PL<D>::
__scramble(D *X_N1, D *X_N2, bool scr_flag, const int frame_id)
{
	std::copy(X_N1, X_N1 + 2 * start_ix, X_N2);

	for(int i = this->start_ix; i < this->N/2; i++)
	{
		int R_lsb     = this->PL_RAND_SEQ[i] % 2;
		int R_msb     = this->PL_RAND_SEQ[i] / 2;
		int R_real    = (1 - R_lsb) * (-2 * R_msb +1); // real part
		int R_imag    = R_lsb       * (-2 * R_msb +1); // imag part
		R_imag        = (2 * scr_flag -1) * R_imag; // conjugate if scr_flag == false, i.e. descrambling
		float D_real  = X_N1[2 * i];
		float D_imag  = X_N1[2 * i +1];
		X_N2[2*i]     = R_real*D_real - R_imag*D_imag; // real part
		X_N2[2*i + 1] = R_imag*D_real + R_real*D_imag; // imag part
	}
}

}
}

#endif

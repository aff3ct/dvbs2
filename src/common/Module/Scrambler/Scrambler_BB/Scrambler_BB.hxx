/*!
 * \file
 * \brief Generates a message.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef SCRAMBLER_BB_HXX_
#define SCRAMBLER_BB_HXX_

#include <algorithm>

namespace aff3ct
{
namespace module
{

template <typename D>
Scrambler_BB<D>::
Scrambler_BB(const int N, const int n_frames)
: Scrambler<D>(N, n_frames)
{
	const std::string name = "Scrambler_BB";
	this->set_name(name);
	this->set_short_name(name);

	lfsr.resize(15);
}

template <typename D>
Scrambler_BB<D>* Scrambler_BB<D>
::clone() const
{
	auto m = new Scrambler_BB(*this);
	m->deep_copy(*this);
	return m;
}

template <typename D>
void Scrambler_BB<D>::
init_lfsr()
{
	for( int i = 0; i < 15; i++ )
		lfsr[i] = lfsr_init[i];
};

template <typename D>
void Scrambler_BB<D>::
_scramble(D *X_N1, D *X_N2, const int frame_id)
{
	init_lfsr();
	for(int i = 0; i < this->N; i++)
	{
		// step on LFSR
		int feedback = (lfsr[14] + lfsr[13]) % 2;
		std::rotate(lfsr.begin(), lfsr.end()-1, lfsr.end());
		lfsr[0] = feedback;

		// apply random bit
		X_N2[i] = (X_N1[i] + feedback) % 2;
	}
}


template <typename D>
void Scrambler_BB<D>::
_descramble(D *Y_N1, D *Y_N2, const int frame_id)
{
	_scramble(Y_N1, Y_N2, frame_id);
}


}
}

#endif

/*!
 * \file
 * \brief Pushes data to a file.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef SCRAMBLER_BB_HPP_
#define SCRAMBLER_BB_HPP_

#include <vector>

#include "Module/Scrambler/Scrambler.hpp"

namespace aff3ct
{
namespace module
{
/*!
 * \class Scrambler_BB (Base Band Scrambler)
 *
 * \brief Randomize data with an LFSR (DVB-S2 standard).
 */
template <typename D = int>
class Scrambler_BB : public Scrambler<D>
{
private:
	const std::vector<int> lfsr_init{1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0};
	std::vector<int> lfsr;

public:
	/*!
	 * \brief Constructor.
	 *
	 * \param N :       frame size
	 * \param n_frames: number of frames to process in the Scrambler_PL.
	 * \param name:     Scrambler_PL's name.
	 */
	Scrambler_BB(const int N, const int n_frames = 1);

	Scrambler_BB();

	virtual Scrambler_BB<D>* clone() const;

protected:
	virtual void _scramble  (D *X_N1, D *X_N2, const int frame_id);
	virtual void _descramble(D *Y_N1, D *Y_N2, const int frame_id);

private:
	void init_lfsr();
};
}
}

#include "Scrambler_BB.hxx"

#endif /* SCRAMBLER_BB_HPP_ */

/*!
 * \file
 * \brief Pushes data to a file.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef BB_SCRAMBLER_HPP
#define BB_SCRAMBLER_HPP

#include <vector>

/*!
 * \class BB_scrambler (Base Band Scrambler)
 *
 * \brief Randomize data with an LFSR (DVB-S2 standard).
 */
class BB_scrambler
{
private:
	const std::vector<int> lfsr_init{1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0};
	std::vector<int> lfsr;

public:
	BB_scrambler();
	void init_lfsr();
	void scramble(std::vector<int>& vec);
};

#endif /* BB_SCRAMBLER_HPP */

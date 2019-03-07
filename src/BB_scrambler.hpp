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
#include <iostream>

/*!
 * \class BB_scrambler (Base Band Scrambler)
 *
 * \brief Randomize data with an LFSR (DVB-S2 standard).
 */
class BB_scrambler
{

private:

	const std::vector<int  > lfsr_init{1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0};	
	std::vector<int  > lfsr;

public:
	
	BB_scrambler()
	{
		lfsr.resize(15);
	}	

	void init_lfsr()
	{
		// init LFSR
		for( int i = 0; i < 14; i++ )
			lfsr[i] = lfsr_init[i];
	};

	void scramble(std::vector<int >& vec)
	{
		init_lfsr();
		for( int i = 0; i < vec.size(); i++)
		{			
			// step on LFSR
			int feedback = (lfsr[14] + lfsr[13]) % 2;
			std::rotate(lfsr.begin(), lfsr.end()-1, lfsr.end());
			lfsr[0] = feedback;
			
			// apply random bit
			vec[i] = (vec[i] + feedback) % 2;		
		}
	};

};
//#include "BB_scrambler.cpp"

#endif /* BB_SCRAMBLER_HPP */

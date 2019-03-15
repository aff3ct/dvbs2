#include <algorithm>

#include "BB_scrambler.hpp"

BB_scrambler
::BB_scrambler()
{
	lfsr.resize(15);
}

void BB_scrambler
::init_lfsr()
{
	// init LFSR
	for( int i = 0; i < 14; i++ )
		lfsr[i] = lfsr_init[i];
};

void BB_scrambler
::scramble(std::vector<int >& vec)
{
	init_lfsr();
	for(size_t i = 0; i < vec.size(); i++)
	{
		// step on LFSR
		int feedback = (lfsr[14] + lfsr[13]) % 2;
		std::rotate(lfsr.begin(), lfsr.end()-1, lfsr.end());
		lfsr[0] = feedback;

		// apply random bit
		vec[i] = (vec[i] + feedback) % 2;
	}
};
#include <cassert>
#include <iostream>
#include <vector>
#include <complex>
#include <cmath>
#include <fstream>      // std::ifstream

#include "Module/Multiplier/Sequence/Multiplier_fading_DVBS2O.hpp"
using namespace aff3ct::module;

template <typename R>
Multiplier_fading_DVBS2O<R>
::Multiplier_fading_DVBS2O(const int N, const std::string& snr_list_filename, R esn0_ref, const int n_frames)
: Multiplier<R>(N, n_frames), gain_sequence(), snr_idx(0)
{
	std::ifstream file(snr_list_filename.c_str());
	R lin_esn0_ref = std::pow(10, esn0_ref/10 );
	if (file.is_open())
	{
		R esn0 = 0.0f;
		while(true)
		{
			file >> esn0;
			if(file.eof())
				break;
			this->gain_sequence.push_back(std::sqrt( std::pow(10, esn0/10) / lin_esn0_ref ));
		}
		file.close();
	}
	else
	{
		this->gain_sequence.push_back((R)1.0);
	}

}
template <typename R>
Multiplier_fading_DVBS2O<R>
::~Multiplier_fading_DVBS2O()
{}

template <typename R>
void Multiplier_fading_DVBS2O<R>
::_imultiply(const R *X_N,  R *Z_N, const int frame_id)
{
	for (auto i = 0 ; i < this->N ; i++)
		Z_N[i] = X_N[i] * this->gain_sequence[this->snr_idx];
	this->snr_idx++;
	this->snr_idx %= this->gain_sequence.size();
}
template class aff3ct::module::Multiplier_fading_DVBS2O<float>;
template class aff3ct::module::Multiplier_fading_DVBS2O<double>;
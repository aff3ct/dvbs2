#ifndef Synchronizer_LR_DVBS2_AIB_HPP
#define Synchronizer_LR_DVBS2_AIB_HPP

#include <vector>
#include <complex>

#include "Module/Synchronizer/Synchronizer_freq/Synchronizer_freq_fine/Synchronizer_freq_fine.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Synchronizer_Luise_Reggiannini_DVBS2_aib : public Synchronizer_freq_fine<R>
{
private:
	R alpha;
	
	int                pilot_nbr;
	std::vector<int>   pilot_start;

	std::vector<R> R_l;
	R n_vals[mipp::N<R>()];

public:
	Synchronizer_Luise_Reggiannini_DVBS2_aib (const int N, const R alpha, const int n_frames = 1);
	virtual ~Synchronizer_Luise_Reggiannini_DVBS2_aib();

protected:
	void _synchronize(const R *X_N1,  R *Y_N2, const int frame_id);
	void _synchronize_old(const R *X_N1,  R *Y_N2, const int frame_id);
	void _reset();

};

}
}

#endif //Synchronizer_Luise_Reggiannini_DVBS2_aib_HPP

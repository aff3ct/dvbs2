#ifndef Synchronizer_LR_DVBS2_AIB_HPP
#define Synchronizer_LR_DVBS2_AIB_HPP

#include <vector>
#include <complex>

#include "Module/Synchronizer/Synchronizer.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Synchronizer_Luise_Reggiannini_DVBS2_aib : public Synchronizer<R>
{
private:
	const int                pilot_size;
	const int                pilot_nbr;
	const std::vector<R>     pilot_values;
	const std::vector<int>   pilot_start;

	R est_reduced_freq;
	std::vector<R> R_l;

public:
	Synchronizer_Luise_Reggiannini_DVBS2_aib (const int N, const std::vector<R> pilot_values, const std::vector<int> pilot_start);
	virtual ~Synchronizer_Luise_Reggiannini_DVBS2_aib();
	void reset();
	R get_est_reduced_freq();

protected:
	void _synchronize(const R *X_N1,  R *Y_N2, const int frame_id);

};

}
}

#endif //Synchronizer_Luise_Reggiannini_DVBS2_aib_HPP

#ifndef SYNCHRONIZER_LR_CC_NAIVE_HPP
#define SYNCHRONIZER_LR_CC_NAIVE_HPP

#include <vector>
#include <complex>

#include "Synchronizer.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Synchronizer_LR_cc_naive : public Synchronizer<R>
{
private:
	const int                pilot_size;
	const int                pilot_nbr;
	const std::vector<R>     pilot_values;
	const std::vector<int>   pilot_start;

	R est_reduced_freq;
	std::vector<R> R_l;
	
public:
	Synchronizer_LR_cc_naive (const int N, const std::vector<R> pilot_values, const std::vector<int> pilot_start);
	virtual ~Synchronizer_LR_cc_naive();
	void reset();
	R get_est_reduced_freq();

protected:
	void _synchronize(const R *X_N1,  R *Y_N2, const int frame_id);

};

}
}

#endif //SYNCHRONIZER_LR_CC_NAIVE_HPP

#ifndef SYNCHRONIZER_FINE_PF_CC_DVBS2O
#define SYNCHRONIZER_FINE_PF_CC_DVBS2O

#include <vector>
#include <complex>

#include "Module/Synchronizer/Synchronizer.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Synchronizer_fine_pf_cc_DVBS2O : public Synchronizer<R>
{
private:
	const int                pilot_size;
	const int                pilot_nbr;
	const std::vector<R>     pilot_values;
	const std::vector<int>   pilot_start;

	R estimated_freq;
	R estimated_phase;

public:
	Synchronizer_fine_pf_cc_DVBS2O (const int N, const std::vector<R> pilot_values, const std::vector<int> pilot_start,
	                                const int n_frames = 1);
	virtual ~Synchronizer_fine_pf_cc_DVBS2O();
	void reset();
	R get_estimated_freq(){return this->estimated_freq;};
	R get_estimated_phase(){return this->estimated_phase;};

protected:
	void _synchronize(const R *X_N1,  R *Y_N2, const int frame_id);

};

}
}

#endif //SYNCHRONIZER_FINE_PF_CC_DVBS2O

#ifndef SYNCHRONIZER_FREQ_PHASE_DVBS2_AIB
#define SYNCHRONIZER_FREQ_PHASE_DVBS2_AIB

#include <vector>
#include <complex>

#include "Module/Synchronizer/Synchronizer_freq/Synchronizer_freq_fine/Synchronizer_freq_fine.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Synchronizer_freq_phase_DVBS2_aib : public Synchronizer_freq_fine<R>
{
private:
	std::vector<int>   pilot_start;

public:
	Synchronizer_freq_phase_DVBS2_aib (const int N, const int n_frames = 1);
	virtual ~Synchronizer_freq_phase_DVBS2_aib();
	virtual Synchronizer_freq_phase_DVBS2_aib<R>* clone() const;

protected:
	void _synchronize(const R *X_N1,  R *Y_N2, const int frame_id);
	void _reset();

};

}
}

#endif //SYNCHRONIZER_FREQ_PHASE_DVBS2_AIB

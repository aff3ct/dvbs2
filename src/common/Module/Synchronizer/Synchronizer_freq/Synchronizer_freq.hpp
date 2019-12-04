#ifndef SYNCHRONIZER_FREQ_HPP
#define SYNCHRONIZER_FREQ_HPP

#include <vector>
#include <complex>

#include "Module/Synchronizer/Synchronizer.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Synchronizer_freq : public Synchronizer<R>
{
public:
	Synchronizer_freq(const int N, const int n_frames = 1);
	virtual ~Synchronizer_freq() = default;

	R get_estimated_freq () {return this->estimated_freq; };
	R get_estimated_phase() {return this->estimated_phase;};

	void reset();

protected:
	R estimated_freq;
	R estimated_phase;

	virtual void _synchronize(const R *X_N1,  R *Y_N2, const int frame_id) = 0;
	virtual void _reset      (                                           ) = 0;
};

}
}
#include "Synchronizer_freq.hxx"
#endif //SYNCHRONIZER_FREQ_HPP

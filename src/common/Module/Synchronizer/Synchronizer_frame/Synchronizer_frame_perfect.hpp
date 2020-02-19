#ifndef SYNCHRONIZER_FRAME_PERFECT
#define SYNCHRONIZER_FRAME_PERFECT

#include <vector>
#include <complex>

#include "Module/Synchronizer/Synchronizer_frame/Synchronizer_frame.hpp"
#include "Module/Filter/Variable_delay/Variable_delay_cc_naive.hpp"
#include "Module/Filter/Filter_FIR/Filter_FIR_ccr.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Synchronizer_frame_perfect : public Synchronizer_frame<R>
{
private:
	const int frame_delay;
	Variable_delay_cc_naive<R> output_delay;

public:
	Synchronizer_frame_perfect (const int N, const int frame_delay, const int n_frames = 1);
	virtual ~Synchronizer_frame_perfect();
	void step(const std::complex<R>* x_elt, R* y_elt);
	void reset();
	R    _get_metric() const;
	bool _get_packet_flag() const;

protected:
	void _synchronize(const R *X_N1, int* delay, R *Y_N2, const int frame_id);
};

}
}

#endif //SYNCHRONIZER_FRAME_PERFECT

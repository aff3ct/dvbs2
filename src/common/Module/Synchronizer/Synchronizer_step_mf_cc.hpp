#ifndef SYNCHRONIZER_STEP_MF_CC_HPP
#define SYNCHRONIZER_STEP_MF_CC_HPP

#include <vector>
#include <complex>
#include <aff3ct.hpp>

#include "Module/Synchronizer/Synchronizer.hpp"
#include "Module/Synchronizer/Synchronizer_Gardner_cc_naive.hpp"
#include "Module/Synchronizer/Synchronizer_coarse_freq/Synchronizer_coarse_freq.hpp"
#include "Module/Filter/Filter_FIR/Filter_RRC/Filter_RRC_ccr_naive.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Synchronizer_step_mf_cc : public Synchronizer<R>
{

public:
	Synchronizer_step_mf_cc (aff3ct::module::Synchronizer_coarse_freq<R>         *sync_coarse_f,
	                         aff3ct::module::Filter_RRC_ccr_naive<R>             *matched_filter,
	                         aff3ct::module::Synchronizer_Gardner_cc_naive<R>    *sync_gardner,
	                         const int n_frames = 1);

	virtual ~Synchronizer_step_mf_cc();
	void reset();

	aff3ct::module::Synchronizer_coarse_freq<R>         *sync_coarse_f;
	aff3ct::module::Filter_RRC_ccr_naive<R>             *matched_filter;
	aff3ct::module::Synchronizer_Gardner_cc_naive<R>    *sync_gardner;

	int get_delay(){return this->gardner_delay;};

protected:
	void _synchronize(const R *X_N1,  R *Y_N2, const int frame_id);
	int gardner_delay;
};

}
}

#endif //SYNCHRONIZER_STEP_MF_CC_HPP

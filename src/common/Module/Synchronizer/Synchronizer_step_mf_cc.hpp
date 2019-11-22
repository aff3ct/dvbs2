#ifndef SYNCHRONIZER_STEP_MF_CC_HPP
#define SYNCHRONIZER_STEP_MF_CC_HPP

#include <vector>
#include <complex>
#include <aff3ct.hpp>

#include "Module/Synchronizer/Synchronizer.hpp"
#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_timing.hpp"
#include "Module/Synchronizer/Synchronizer_freq/Synchronizer_freq_coarse/Synchronizer_freq_coarse.hpp"
#include "Module/Filter/Filter_FIR/Filter_RRC/Filter_RRC_ccr_naive.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Synchronizer_step_mf_cc : public Synchronizer<R>
{

public:
	Synchronizer_step_mf_cc (aff3ct::module::Synchronizer_freq_coarse<R> *sync_coarse_f,
	                         aff3ct::module::Filter_RRC_ccr_naive<R>     *matched_filter,
							 aff3ct::module::Synchronizer_timing<R>      *sync_timing);

	virtual ~Synchronizer_step_mf_cc();
	void reset();

	aff3ct::module::Synchronizer_freq_coarse<R> *sync_coarse_f;
	aff3ct::module::Filter_RRC_ccr_naive<R>     *matched_filter;
	aff3ct::module::Synchronizer_timing<R>      *sync_timing;

	int get_delay(){return this->sync_timing->get_delay();};

protected:
	void _synchronize(const R *X_N1,  R *Y_N2, const int frame_id);
};

}
}

#endif //SYNCHRONIZER_STEP_MF_CC_HPP

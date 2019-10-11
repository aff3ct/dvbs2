#ifndef SYNCHRONIZER_STEP_MF_CC_HPP
#define SYNCHRONIZER_STEP_MF_CC_HPP

#include <vector>
#include <complex>
#include <aff3ct.hpp>

#include "../Params_DVBS2O/Params_DVBS2O.hpp"

#include "Synchronizer.hpp"
#include "Synchronizer_Gardner_cc_naive.hpp"
#include "Synchronizer_coarse_fr_cc_DVBS2O.hpp"
#include "../Filter/Filter_FIR/Filter_RRC/Filter_RRC_ccr_naive.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Synchronizer_step_mf_cc : public Synchronizer<R>
{
	
public:
	Synchronizer_step_mf_cc (aff3ct::module::Synchronizer_coarse_fr_cc_DVBS2O<R> *sync_coarse_f,
	                         aff3ct::module::Filter_RRC_ccr_naive<R>             *matched_filter,
							 aff3ct::module::Synchronizer_Gardner_cc_naive<R>    *sync_gardner);

	virtual ~Synchronizer_step_mf_cc();
	void reset();
	
	aff3ct::module::Synchronizer_coarse_fr_cc_DVBS2O<R> *sync_coarse_f;
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

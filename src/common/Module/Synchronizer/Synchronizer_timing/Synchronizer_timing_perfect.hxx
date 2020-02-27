#include "aff3ct.hpp"
#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_timing_perfect.hpp"

namespace aff3ct
{
namespace module
{

template <typename B, typename R>
void Synchronizer_timing_perfect<B,R>
::step(const std::complex<R> *X_N1, std::complex<R> *Y_N1, B *B_N1)
{
	farrow_flt.step( X_N1, Y_N1);
	B_N1[0] = this->is_strobe;
	B_N1[1] = this->is_strobe;
	this->last_symbol = (this->is_strobe == 1)?*Y_N1:this->last_symbol;

	this->NCO_counter += 1.0f;
	this->NCO_counter = (R)((int)this->NCO_counter % this->osf);

	this->is_strobe = ((int)this->NCO_counter % this->osf == 0) ? 1:0; // Check if a strobe
}

}
}

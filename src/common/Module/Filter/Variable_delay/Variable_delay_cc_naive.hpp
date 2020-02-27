#ifndef VARIABLE_DELAY_CC_NAIVE_HPP
#define VARIABLE_DELAY_CC_NAIVE_HPP

#include <vector>
#include <complex>

#include "Module/Filter/Filter.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Variable_delay_cc_naive : public Filter<R>
{
private:
	int delay;
	std::vector<std::complex<R> > buff;
	std::vector<R> buff2;
	int head;
	int head2;
	int size;
	bool first_time;

public:
	Variable_delay_cc_naive (const int N, const int delay, const int max_delay, const int n_frames = 1);
	virtual ~Variable_delay_cc_naive();
	inline void step(const std::complex<R>* x_elt, std::complex<R>* y_elt);
	void reset();
	void set_delay(const int delay);

protected:
	void _filter(const R *X_N1,  R *Y_N2, const int frame_id);
	void _filter_old(const R *X_N1,  R *Y_N2, const int frame_id);

};

}
}

#include "Module/Filter/Variable_delay/Variable_delay_cc_naive.hxx"

#endif //VARIABLE_DELAY_CC_NAIVE_HPP

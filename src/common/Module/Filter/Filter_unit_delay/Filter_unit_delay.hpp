#ifndef FILTER_UNIT_DELAY
#define FILTER_UNIT_DELAY

#include <vector>
#include <complex>

#include "Module/Filter/Filter.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = int>
class Filter_unit_delay : public Filter<R>
{
private:
	std::vector<R > mem;

public:
	Filter_unit_delay (const int N, const int n_frames = 1);
	virtual ~Filter_unit_delay();

	void reset();

protected:
	void _filter(const R *X_N1,  R *Y_N2, const int frame_id);

};
}
}

#endif //FILTER_UNIT_DELAY

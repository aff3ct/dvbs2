#ifndef FILTER_BUFFERED_DELAY
#define FILTER_BUFFERED_DELAY

#include <vector>
#include <complex>

#include "Module/Filter/Filter.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = int>
class Filter_buffered_delay : public Filter<R>
{
private:
	std::vector<std::vector<R>> mem;
	std::vector<R*> mem_heads;

public:
	Filter_buffered_delay (const int N, const int n_frames = 1);
	virtual ~Filter_buffered_delay();

	void reset();

protected:
	int delay;
	void _filter(const R *X_N1,  R *Y_N2, const int frame_id);

};
}
}

#endif //FILTER_BUFFERED_DELAY

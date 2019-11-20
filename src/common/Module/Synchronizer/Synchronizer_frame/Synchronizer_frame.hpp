#ifndef SYNCHRONIZER_FRAME_HPP
#define SYNCHRONIZER_FRAME_HPP

#include <vector>
#include <complex>

#include "Module/Synchronizer/Synchronizer.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Synchronizer_frame : public Synchronizer<R>
{

public:
	Synchronizer_frame(const int N);
	virtual ~Synchronizer_frame() = default;
	virtual void reset() = 0;
	
	int get_delay(){return this->delay;};
protected:
	virtual void _synchronize(const R *X_N1,  R *Y_N2, const int frame_id) = 0;
	int delay;

};

}
}
#include "Synchronizer_frame.hxx"
#endif //SYNCHRONIZER_FRAME_HPP

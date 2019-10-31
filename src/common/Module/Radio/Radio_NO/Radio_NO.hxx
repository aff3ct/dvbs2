#include <typeinfo>

namespace aff3ct
{
namespace module
{

template <typename R>
Radio_NO<R>::
Radio_NO(const int N, const int n_frames)
: Radio<R>(N, n_frames)
{
}

template <typename R>
void Radio_NO<R>::
_send(const R *X_N1, const int frame_id)
{
}

template <typename R>
void Radio_NO<R>::
_receive(R *Y_N1, const int frame_id)
{
	std::fill(Y_N1, Y_N1 + this->N, (R)0);
}

}
}

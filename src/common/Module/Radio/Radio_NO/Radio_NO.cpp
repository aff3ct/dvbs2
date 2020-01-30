#include <typeinfo>

#include "Module/Radio/Radio_NO/Radio_NO.hpp"

using namespace aff3ct;
using namespace aff3ct::module;

template <typename R>
Radio_NO<R>
::Radio_NO(const int N, const int n_frames)
: Radio<R>(N, n_frames)
{
	const std::string name = "Radio_NO";
	this->set_name(name);
}

template <typename R>
void Radio_NO<R>
::_send(const R *X_N1, const int frame_id)
{
}

template <typename R>
void Radio_NO<R>
::_receive(R *Y_N1, const int frame_id)
{
	std::fill(Y_N1, Y_N1 + this->N, (R)0);
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Radio_NO<double>;
template class aff3ct::module::Radio_NO<float>;
template class aff3ct::module::Radio_NO<int16_t>;
template class aff3ct::module::Radio_NO<int8_t>;
// ==================================================================================== explicit template instantiation

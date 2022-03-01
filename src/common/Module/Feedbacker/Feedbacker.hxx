#ifndef FEEDBACKER_HXX_
#define FEEDBACKER_HXX_

#include <sstream>
#include <complex>

#include "Tools/Exception/exception.hpp"

#include "Module/Feedbacker/Feedbacker.hpp"

namespace aff3ct
{
namespace module
{

template <typename D>
Feedbacker<D>
::Feedbacker(const int N, const D init_val)
: Module(), N(N), init_val(init_val), data(this->N * this->n_frames, init_val)
{
	const std::string name = "Feedbacker";
	this->set_name(name);
	this->set_short_name(name);

	if (N <= 0)
	{
		std::stringstream message;
		message << "'N' has to be greater than 0 ('N' = " << N << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	auto &p1 = this->create_task("memorize");
	auto p1s_X_N = this->template create_socket_in <D>(p1, "X_N", this->N);
	this->create_codelet(p1, [p1s_X_N](Module &m, Task &t, const size_t frame_id) -> int
	{
		static_cast<Feedbacker<D>&>(m)._memorize(static_cast<D*>(t[p1s_X_N].get_dataptr()), frame_id);
		return 0;
	});

	auto &p2 = this->create_task("produce");
	auto p2s_Y_N = this->template create_socket_out<D>(p2, "Y_N", this->N);
	this->create_codelet(p2, [p2s_Y_N](Module &m, Task &t, const size_t frame_id) -> int
	{
		static_cast<Feedbacker<D>&>(m)._produce(static_cast<D*>(t[p2s_Y_N].get_dataptr()), frame_id);
		return 0;
	});
}

template <typename D>
Feedbacker<D>* Feedbacker<D>
::clone() const
{
	auto m = new Feedbacker(*this);
	m->deep_copy(*this);
	return m;
}

template <typename D>
int Feedbacker<D>
::get_N() const
{
	return N;
}

template <typename D>
void Feedbacker<D>
::_memorize(const D *X_N, const size_t frame_id)
{
	std::copy(X_N,
	          X_N + this->N,
	          this->data.data() + this->N * frame_id);
}

template <typename D>
void Feedbacker<D>
::_produce(D *Y_N, const size_t frame_id)
{
	std::copy(this->data.data() + this->N * (frame_id +0),
	          this->data.data() + this->N * (frame_id +1),
	          Y_N);
}

template<typename D>
void Feedbacker<D>
::set_n_frames(const size_t n_frames)
{
	const auto old_n_frames = this->get_n_frames();
	if (old_n_frames != n_frames)
	{
		Module::set_n_frames(n_frames);

		const auto old_data_size = this->data.size();
		const auto new_data_size = (old_data_size / old_n_frames) * n_frames;
		this->data.resize(new_data_size, this->init_val);
	}
}

}
}

#endif

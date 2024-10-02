#include "Tools/Exception/exception.hpp"

#include "Module/Estimator/Estimator_perfect.hpp"

namespace aff3ct
{
namespace module
{

template <typename R>
Estimator_perfect<R>::
Estimator_perfect(const int N, tools::Sigma<R>* noise_ref, const int n_frames)
: Estimator<R>(N, n_frames), noise_ref(noise_ref)
{
	for (auto& t : this->tasks)
		t->set_replicability(true);
}

template <typename R>
Estimator_perfect<R>* Estimator_perfect<R>
::clone() const
{
	auto m = new Estimator_perfect(*this);
	m->deep_copy(*this);
	return m;
}

template <typename R>
void Estimator_perfect<R>
::set_noise_ref(tools::Sigma<>& noise)
{
	this->noise_ref = &noise;
}

template<typename R>
tools::Sigma<>& Estimator_perfect<R>
::get_noise_ref() const
{
	if (this->noise_ref == nullptr)
	{
		std::stringstream message;
		message << "'noise' should not be nullptr.";
		throw spu::tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
	}

	return *this->noise_ref;
}

template <typename R>
void Estimator_perfect<R>
::_estimate(const R *X_N, const int frame_id)
{
	if (this->noise_ref == nullptr)
	{
		std::stringstream message;
		message << "'noise_ref' should not be nullptr.";
		throw spu::tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
	}

	this->sigma_estimated = this->noise_ref->get_value();
	this->ebn0_estimated  = this->noise_ref->get_ebn0();
	this->esn0_estimated  = this->noise_ref->get_esn0();
}

}
}

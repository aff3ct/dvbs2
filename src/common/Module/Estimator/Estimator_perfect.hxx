#include "Tools/Exception/exception.hpp"

#include "Module/Estimator/Estimator_perfect.hpp"

namespace aff3ct
{
namespace module
{

template <typename R>
Estimator_perfect<R>::
Estimator_perfect(const int N, tools::Noise<R>* noise_ref, const int n_frames)
: Estimator<R>(N, n_frames), noise_ref(nullptr)
{
	set_noise_ref(*noise_ref);
}

template <typename R>
Estimator_perfect<R>* Estimator_perfect<R>
::clone() const
{
	auto m = new Estimator_perfect(*this);
	m->deep_copy(*this);
	return m;
}

template<typename R>
void Estimator_perfect<R>
::check_noise()
{
	Estimator<R>::check_noise();
}

template<typename R>
void Estimator_perfect<R>
::check_noise_ref()
{
	if (this->noise_ref == nullptr)
	{
		std::stringstream message;
		message << "'noise' should not be nullptr.";
		throw tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
	}
}

template <typename R>
void Estimator_perfect<R>
::set_noise_ref(tools::Noise<>& noise)
{
	this->noise_ref = &noise;
	this->check_noise_ref();
	this->noise_ref->record_callback_update([this](){ this->notify_noise_update(); });
	if (this->noise_ref->is_set())
		this->notify_noise_update();
}

template <typename R>
void Estimator_perfect<R>
::notify_noise_update()
{
	this->check_noise();
	this->noise->copy(*noise_ref);
}

template<typename R>
tools::Noise<>& Estimator_perfect<R>
::get_noise_ref() const
{
	this->check_noise_ref();
	return *this->noise_ref;
}

template <typename R>
void Estimator_perfect<R>::
_estimate(const R *X_N, R *H_N, const int frame_id)
{
	this->check_noise();
	this->check_noise_ref();

	for (int i = 0; i < this->N / 2; i++)
	{
		H_N[2*i]   = 1.0;
		H_N[2*i+1] = 0.0;
	}
	tools::Sigma<R> * sigma = dynamic_cast<tools::Sigma<R>*>(this->noise_ref);
	this->sigma_estimated = sigma->get_value();
	this->ebn0_estimated = sigma->get_ebn0();
	this->esn0_estimated = sigma->get_esn0();
}

template <typename R>
void Estimator_perfect<R>::
_rescale(const R *X_N, R *H_N, R *Y_N, const int frame_id)
{
	this->check_noise();
	this->check_noise_ref();

	std::copy(X_N, X_N + this->N, Y_N);
	tools::Sigma<R> * sigma = dynamic_cast<tools::Sigma<R>*>(this->noise_ref);
	R _sigma = sigma->get_value();

	for (int i = 0; i < this->N / 2; i++)
	{
		Y_N[2*i]   /= _sigma;
		Y_N[2*i+1] /= _sigma;
	}

	float H_ = 1/_sigma; std::sqrt(2*sigma->get_esn0());
	for (int i = 0; i < this->N / 2; i++)
	{
		H_N[2*i]   = H_;
		H_N[2*i+1] = 0.0;
	}

	this->sigma_estimated = sigma->get_value();
	this->ebn0_estimated = sigma->get_ebn0();
	this->esn0_estimated = sigma->get_esn0();
}

}
}

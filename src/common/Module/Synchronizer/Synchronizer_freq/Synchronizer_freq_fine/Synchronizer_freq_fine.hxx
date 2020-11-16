#ifndef SYNCHRONIZER_FREQ_FINE_HXX_
#define SYNCHRONIZER_FREQ_FINE_HXX_

#include <string>
#include <memory>
#include <stdexcept>
#include <cmath>
#include <sstream>

#include "Tools/Exception/exception.hpp"

namespace aff3ct
{
namespace module
{
	template <typename R>
	Synchronizer_freq_fine<R>::
	Synchronizer_freq_fine(const int N, const int n_frames)
	: Module(), N(N), estimated_freq((R)0.0), estimated_phase((R)0.0)
	{
		const std::string name = "Synchronizer_freq_fine";
		this->set_name(name);
		this->set_short_name(name);
		this->set_n_frames(n_frames);
		this->set_single_wave(true);

		if (N <= 0)
		{
			std::stringstream message;
			message << "'N' has to be greater than 0 ('N' = " << N << ").";
			throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
		}

		auto &p1 = this->create_task("synchronize");
		auto p1s_X_N1 = this->template create_socket_in <R>(p1, "X_N1", this->N );
		auto p1s_FRQ  = this->template create_socket_out<R>(p1, "FRQ" , 1       );
		auto p1s_PHS  = this->template create_socket_out<R>(p1, "PHS" , 1       );
		auto p1s_Y_N2 = this->template create_socket_out<R>(p1, "Y_N2", this->N);
		this->create_codelet(p1, [p1s_X_N1, p1s_FRQ, p1s_PHS, p1s_Y_N2](Module &m, Task &t, const size_t frame_id) -> int
		{
			static_cast<Synchronizer_freq_fine<R>&>(m).synchronize(static_cast<R*>(t[p1s_X_N1].get_dataptr()),
			                                                       static_cast<R*>(t[p1s_FRQ] .get_dataptr()),
			                                                       static_cast<R*>(t[p1s_PHS] .get_dataptr()),
			                                                       static_cast<R*>(t[p1s_Y_N2].get_dataptr()));

			return 0;
		});
	}

	template <typename R>
	Synchronizer_freq_fine<R>* Synchronizer_freq_fine<R>
	::clone() const
	{
		throw tools::unimplemented_error(__FILE__, __LINE__, __func__);
	}

	template <typename R>
	int Synchronizer_freq_fine<R>::
	get_N() const
	{
		return this->N;
	}

	template <typename R>
	void Synchronizer_freq_fine<R>
	::reset()
	{
		this->estimated_freq  = (R)0.0;
		this->estimated_phase = (R)0.0;
		this->_reset();
	};

	template <typename R>
	template <class AR>
	void Synchronizer_freq_fine<R>::
	synchronize(const std::vector<R,AR>& X_N1, std::vector<R,AR>& FRQ, std::vector<R,AR>& PHS, std::vector<R,AR>& Y_N2, const int frame_id)
	{
		if (this->N * this->n_frames != (int)X_N1.size())
		{
			std::stringstream message;
			message << "'X_N1.size()' has to be equal to 'N' * 'n_frames' ('X_N1.size()' = " << X_N1.size()
					<< ", 'N' = " << this->N << ", 'n_frames' = " << this->n_frames << ").";
			throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
		}

		if (this->n_frames != (int)FRQ.size())
		{
			std::stringstream message;
			message << "'FRQ.size()' has to be equal to 'n_frames' ('FRQ.size()' = " << FRQ.size()
					<< ", 'n_frames' = " << this->n_frames << ").";
			throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
		}

		if (this->n_frames != (int)PHS.size())
		{
			std::stringstream message;
			message << "'PHS.size()' has to be equal to 'n_frames' ('PHS.size()' = " << PHS.size()
					<< ", 'n_frames' = " << this->n_frames << ").";
			throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
		}

		if (this->N * this->n_frames != (int)Y_N2.size())
		{
			std::stringstream message;
			message << "'Y_N2.size()' has to be equal to 'N_fil' * 'n_frames' ('Y_N2.size()' = " << Y_N2.size()
					<< ", 'N' = " << this->N << ", 'n_frames' = " << this->n_frames << ").";
			throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
		}

		this->synchronize(X_N1.data(), FRQ.data(), PHS.data(), Y_N2.data(), frame_id);
	}

	template <typename R>
	void Synchronizer_freq_fine<R>::
	synchronize(const R *X_N1, R *FRQ, R* PHS, R *Y_N2, const int frame_id)
	{
		const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
		const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

		for (auto f = f_start; f < f_stop; f++)
		{
			this->_synchronize(X_N1 + f * this->N,
			                   Y_N2 + f * this->N,
			                   f);
			FRQ[f] = this->estimated_freq;
			PHS[f] = this->estimated_phase;
		}
	}

	template <typename R>
	void Synchronizer_freq_fine<R>::
	_synchronize(const R *X_N1, R *Y_N2, const int frame_id)
	{
		throw tools::unimplemented_error(__FILE__, __LINE__, __func__);
	}
}
}

#endif

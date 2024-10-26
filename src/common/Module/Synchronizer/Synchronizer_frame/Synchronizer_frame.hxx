#ifndef SYNCHRONIZER_FRAME_HXX_
#define SYNCHRONIZER_FRAME_HXX_

#include <string>
#include <memory>
#include <stdexcept>
#include <cmath>
#include <sstream>

namespace aff3ct
{
namespace module
{

template <typename R>
Synchronizer_frame<R>::
Synchronizer_frame(const int N, const int n_frames)
: spu::module::Stateful(), N_in(N), N_out(N), delay(0)
{
	const std::string name = "Synchronizer_frame";
	this->set_name(name);
	this->set_short_name(name);
	this->set_n_frames(n_frames);
	this->set_single_wave(true);

	if (N_in <= 0)
	{
		std::stringstream message;
		message << "'N_in' has to be greater than 0 ('N_in' = " << N_in << ").";
		throw spu::tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	if (N_out <= 0)
	{
		std::stringstream message;
		message << "'N_out' has to be greater than 0 ('N_out' = " << N_out << ").";
		throw spu::tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	auto &p1 = this->create_task("synchronize");
	auto p1s_X_N1 = this->template create_socket_in <R>  (p1, "X_N1" , this->N_in );
	auto p1s_DEL  = this->template create_socket_out<int>(p1, "DEL", 1            );
	auto p1s_FLG  = this->template create_socket_out<int>(p1, "FLG", 1            );
	auto p1s_TRI  = this->template create_socket_out<R>  (p1, "TRI", 1            );
	auto p1s_Y_N2 = this->template create_socket_out<R>  (p1, "Y_N2" , this->N_out);
	this->create_codelet(p1, [p1s_X_N1, p1s_DEL, p1s_FLG, p1s_TRI, p1s_Y_N2](spu::module::Module &m,
	                                                                         spu::runtime::Task &t,
	                                                                         const size_t frame_id) -> int
	{
		static_cast<Synchronizer_frame<R>&>(m).synchronize(static_cast<R*  >(t[p1s_X_N1 ].get_dataptr()),
		                                                   static_cast<int*>(t[p1s_DEL  ].get_dataptr()),
		                                                   static_cast<int*>(t[p1s_FLG  ].get_dataptr()),
		                                                   static_cast<R*  >(t[p1s_TRI  ].get_dataptr()),
		                                                   static_cast<R*  >(t[p1s_Y_N2 ].get_dataptr()));

		return 0;
	});

	auto &p2 = this->create_task("synchronize1");
	auto p2s_X_N1     = this->template create_socket_in <R>(p2, "X_N1",     this->N_in);
	auto p2s_cor_SOF  = this->template create_socket_out<R>(p2, "cor_SOF",  this->N_in);
	auto p2s_cor_PLSC = this->template create_socket_out<R>(p2, "cor_PLSC", this->N_in);
	this->create_codelet(p2, [p2s_X_N1, p2s_cor_SOF, p2s_cor_PLSC](spu::module::Module &m,
	                                                               spu::runtime::Task &t,
	                                                               const size_t frame_id) -> int
	{
		static_cast<Synchronizer_frame<R>&>(m).synchronize1(static_cast<R*>(t[p2s_X_N1    ].get_dataptr()),
		                                                    static_cast<R*>(t[p2s_cor_SOF ].get_dataptr()),
		                                                    static_cast<R*>(t[p2s_cor_PLSC].get_dataptr()));

		return 0;
	});

	auto &p3 = this->create_task("synchronize2");
	auto p3s_X_N1     = this->template create_socket_in <R>  (p3, "X_N1",     this->N_in);
	auto p3s_cor_SOF  = this->template create_socket_in <R>  (p3, "cor_SOF",  this->N_in);
	auto p3s_cor_PLSC = this->template create_socket_in <R>  (p3, "cor_PLSC", this->N_in);
	auto p3s_DEL      = this->template create_socket_out<int>(p3, "DEL", 1              );
	auto p3s_FLG      = this->template create_socket_out<int>(p3, "FLG", 1              );
	auto p3s_TRI      = this->template create_socket_out<R>  (p3, "TRI", 1              );
	auto p3s_Y_N2     = this->template create_socket_out<R>  (p3, "Y_N2",    this->N_out);
	this->create_codelet(p3, [p3s_X_N1, p3s_cor_SOF, p3s_cor_PLSC, p3s_DEL, p3s_FLG, p3s_TRI, p3s_Y_N2]
	    (spu::module::Module &m, spu::runtime::Task &t, const size_t frame_id) -> int
	{
		static_cast<Synchronizer_frame<R>&>(m).synchronize2(static_cast<R*  >(t[p3s_X_N1    ].get_dataptr()),
		                                                    static_cast<R*  >(t[p3s_cor_SOF ].get_dataptr()),
		                                                    static_cast<R*  >(t[p3s_cor_PLSC].get_dataptr()),
		                                                    static_cast<int*>(t[p3s_DEL     ].get_dataptr()),
		                                                    static_cast<int*>(t[p3s_FLG     ].get_dataptr()),
		                                                    static_cast<R*  >(t[p3s_TRI     ].get_dataptr()),
		                                                    static_cast<R*  >(t[p3s_Y_N2    ].get_dataptr()));

		return 0;
	});
}

template <typename R>
int Synchronizer_frame<R>::
get_N_in() const
{
	return this->N_in;
}

template <typename R>
int Synchronizer_frame<R>::
get_N_out() const
{
	return this->N_out;
}

template <typename R>
int Synchronizer_frame<R>::
get_delay() const
{
	return this->delay;
}

template <typename R>
template <class AR>
void Synchronizer_frame<R>::
synchronize(const std::vector<R,AR>& X_N1, std::vector<int>& DEL, std::vector<int>& FLG, std::vector<R,AR>& TRI, std::vector<R,AR>& Y_N2, const int frame_id)
{
	if (this->N_in * this->n_frames != (int)X_N1.size())
	{
		std::stringstream message;
		message << "'X_N1.size()' has to be equal to 'N' * 'n_frames' ('X_N1.size()' = " << X_N1.size()
		        << ", 'N' = " << this->N_in << ", 'n_frames' = " << this->n_frames << ").";
		throw spu::tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (this->n_frames != (int)DEL.size())
	{
		std::stringstream message;
		message << "'DEL.size()' has to be equal to '1' * 'n_frames' ('DEL.size()' = " << DEL.size()
		        << ", 'N' = " << this->N_out << ", 'n_frames' = " << this->n_frames << ").";
		throw spu::tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (this->n_frames != (int)FLG.size())
	{
		std::stringstream message;
		message << "'FLG.size()' has to be equal to '1' * 'n_frames' ('FLG.size()' = " << FLG.size()
		        << ", 'N' = " << this->N_out << ", 'n_frames' = " << this->n_frames << ").";
		throw spu::tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (this->n_frames != (int)TRI.size())
	{
		std::stringstream message;
		message << "'TRI.size()' has to be equal to '1' * 'n_frames' ('TRI.size()' = " << TRI.size()
		        << ", 'N' = " << this->N_out << ", 'n_frames' = " << this->n_frames << ").";
		throw spu::tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (this->N_out * this->n_frames != (int)Y_N2.size())
	{
		std::stringstream message;
		message << "'Y_N2.size()' has to be equal to 'N' * 'n_frames' ('Y_N2.size()' = " << Y_N2.size()
		        << ", 'N' = " << this->N_out << ", 'n_frames' = " << this->n_frames << ").";
		throw spu::tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	this->synchronize(X_N1.data(), DEL.data(), FLG.data(), TRI.data(), Y_N2.data(), frame_id);
}

template <typename R>
void Synchronizer_frame<R>::
synchronize(const R *X_N1, int* DEL, int* FLG, R *TRI, R *Y_N2, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
	{
		this->_synchronize(X_N1 + f * this->N_in,
		                   DEL + f,
		                   Y_N2 + f * this->N_out,
		                   f);

		FLG[f] = (int)this->get_packet_flag();
		TRI[f] = this->get_metric();
	}
}

template <typename R>
void Synchronizer_frame<R>::
synchronize1(const R *X_N1, R *cor_SOF, R *cor_PLSC, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_synchronize1(X_N1 + f * this->N_in,
		                    cor_SOF + f * this->N_out,
		                    cor_PLSC + f * this->N_out,
		                    f);
}

template <typename R>
void Synchronizer_frame<R>::
synchronize2(const R *X_N1, const R *cor_SOF, const R *cor_PLSC, int* DEL, int* FLG, R *TRI, R *Y_N2,
             const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
	{
		this->_synchronize2(X_N1 + f * this->N_in,
		                    cor_SOF + f * this->N_in,
		                    cor_PLSC + f * this->N_in,
		                    DEL + f,
		                    Y_N2 + f * this->N_out,
		                    f);

		FLG[f] = (int)this->get_packet_flag();
		TRI[f] = this->get_metric();
	}
}

template <typename R>
void Synchronizer_frame<R>::
_synchronize(const R *X_N1, int* DEL, R *Y_N2, const int frame_id)
{
	throw spu::tools::unimplemented_error(__FILE__, __LINE__, __func__);
}

template <typename R>
void Synchronizer_frame<R>::
_synchronize1(const R *X_N1, R *cor_SOF, R *cor_PLSC, const int frame_id)
{
	throw spu::tools::unimplemented_error(__FILE__, __LINE__, __func__);
}

template <typename R>
void Synchronizer_frame<R>::
_synchronize2(const R *X_N1, const R *cor_SOF, const R *cor_PLSC, int* DEL, R *Y_N2, const int frame_id)
{
	throw spu::tools::unimplemented_error(__FILE__, __LINE__, __func__);
}

}
}

#endif

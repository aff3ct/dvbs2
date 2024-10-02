/*!
 * \file
 * \brief Transmit or receive data to or from a radio module.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef RADIO_HPP
#define RADIO_HPP

#include <streampu.hpp>

#include "Tools/Interface/Interface_is_done.hpp"
#include "Tools/Interface/Interface_reset.hpp"

namespace aff3ct
{
namespace module
{
	namespace rad
	{
		enum class tsk : uint8_t { send, receive, SIZE };

		namespace sck
		{
			enum class send    : uint8_t { X_N1, status };
			enum class receive : uint8_t { OVF, SEQ, CLT, TIM, Y_N1, status };
		}
	}


/*!
 * \class Radio
 *
 * \brief Transmit or receive data to or from a radio module.
 *
 * \tparam R: type of the data to send or receive.
 *
 */
template <typename R = float>
class Radio : public spu::module::Stateful, public spu::tools::Interface_is_done, public spu::tools::Interface_reset
{
public:
	inline spu::runtime::Task&   operator[](const rad::tsk          t) { return spu::module::Module::operator[]((int)t);                         }
	inline spu::runtime::Socket& operator[](const rad::sck::send    s) { return spu::module::Module::operator[]((int)rad::tsk::send  )[(int)s];  }
	inline spu::runtime::Socket& operator[](const rad::sck::receive s) { return spu::module::Module::operator[]((int)rad::tsk::receive)[(int)s]; }

protected:
	const int N; /*!< Size of one frame (= number of samples in one frame) */
	std::vector<int32_t> ovf_flags;
	std::vector<int32_t> seq_flags;
	std::vector<int32_t> clt_flags;
	std::vector<int32_t> tim_flags;

public:
	/*!
	 * \brief Constructor.
	 *
	 * \param N:     Radio_frame length.
	 */
	Radio(const int N, const int n_frames = 1);

	/*!
	 * \brief Destructor.
	 */
	virtual ~Radio() = default;

	//virtual int get_K() const;

	/*!
	 * \brief Send a frame.
	 *
	 * \param X_N1 : a vector of complex samples to send.
	 */
	template <class A = std::allocator<R>>
	void send(const std::vector<R,A>& X_N1, const int frame_id = -1);

	virtual void send(const R *X_N1, const int frame_id = -1);

	/*!
	 * \brief Receive a frame.
	 *
	 * \param Y_N1 : a vector of complex samples to receive.
	 */
	template <class A = std::allocator<R>>
	void receive(std::vector<int32_t>& OVF,
	             std::vector<int32_t>& SEQ,
	             std::vector<int32_t>& CLT,
	             std::vector<int32_t>& TIM,
	             std::vector<R,A>& Y_N1,
	             const int frame_id = -1);

	virtual void receive(int32_t *OVF,
	                     int32_t *SEQ,
	                     int32_t *CLT,
	                     int32_t *TIM,
	                     R *Y_N1,
	                     const int frame_id = -1);

	virtual bool is_done() const;

	virtual void reset();

protected:
	virtual void _send   (const R *X_N1, const int frame_id) = 0;
	virtual void _receive(      R *Y_N1, const int frame_id) = 0;
};
}
}
#include "Radio.hxx"

#endif /* RADIO_HPP */

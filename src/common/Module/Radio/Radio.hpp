/*!
 * \file
 * \brief Transmit or receive data to or from a radio module.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef RADIO_HPP
#define RADIO_HPP

#include "Module/Module.hpp"


namespace aff3ct
{
namespace module
{
	namespace rad
	{
		enum class tsk : uint8_t { send, receive, SIZE };

		namespace sck
		{
			enum class send    : uint8_t { X_N1, SIZE };
			enum class receive : uint8_t { Y_N1, SIZE };
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
class Radio : public Module
{
public:
	inline Task&   operator[](const rad::tsk          t) { return Module::operator[]((int)t);                         }
	inline Socket& operator[](const rad::sck::send    s) { return Module::operator[]((int)rad::tsk::send  )[(int)s];  }
	inline Socket& operator[](const rad::sck::receive s) { return Module::operator[]((int)rad::tsk::receive)[(int)s]; }

protected:
	const int N;     /*!< Size of one frame (= number of samples in one frame) */

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
	void receive(std::vector<R,A>& Y_N1, const int frame_id = -1);

	virtual void receive(R *Y_N1, const int frame_id = -1);


protected:
	virtual void _send   (const R *X_N1, const int frame_id) = 0;
	virtual void _receive(      R *Y_N1, const int frame_id) = 0;
};
}
}
#include "Radio.hxx"

#endif /* RADIO_HPP */

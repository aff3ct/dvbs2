/*!
 * \file
 * \brief Send data into outer world.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef SINK_HPP
#define SINK_HPP

#include "Module/Module.hpp"


namespace aff3ct
{
namespace module
{
	namespace snk
	{
		enum class tsk : uint8_t { send, SIZE };

		namespace sck
		{
			enum class send : uint8_t { X_N1, SIZE };
		}
	}


/*!
 * \class Sink
 *
 * \brief Send data into outer world.
 *
 * \tparam B: type of the data to send or receive.
 *
 */
template <typename B = int>
class Sink : public Module
{
public:
	inline Task&   operator[](const snk::tsk          t) { return Module::operator[]((int)t);                       }
	inline Socket& operator[](const snk::sck::send    s) { return Module::operator[]((int)snk::tsk::send)[(int)s];  }

protected:
	const int N;     /*!< Size of one frame (= number of samples in one frame) */

public:
	/*!
	 * \brief Constructor.
	 *
	 * \param N:     Radio_frame length.
	 */
	Sink(const int N, const int n_frames = 1);

	/*!
	 * \brief Destructor.
	 */
	virtual ~Sink() = default;

	/*!
	 * \brief Consume a frame.
	 *
	 * \param X_N1 : a vector of data to consume.
	 */
	template <class A = std::allocator<B>>
	void send(const std::vector<B,A>& X_N1, const int frame_id = -1);

	virtual void send(const B *X_N1, const int frame_id = -1);

protected:
	virtual void _send(const B *X_N1, const int frame_id) = 0;
};
}
}
#include "Sink.hxx"

#endif /* SINK_HPP */

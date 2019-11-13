/*!
 * \file
 * \brief Send data to a binary file
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef SINK_BINARY_HPP
#define SINK_BINARY_HPP

#include "Module/Sink/Sink.hpp"

namespace aff3ct
{
namespace module
{
/*!
 * \class Sink_binary
 *
 * \brief Send data to a binary file.
 *
 * \tparam B: type of the data to send or receive.
 *
 */
template <typename B = int>
class Sink_binary : public Sink<B>
{
private:
	std::ofstream sink_file;

public:
	/*!
	 * \brief Constructor.
	 *
	 * \param N:     Radio_frame length.
	 * \param n_frames:     number of frames.
	 */
	Sink_binary(const int N, std::string filename, const int n_frames = 1);

	/*!
	 * \brief Destructor.
	 */
	~Sink_binary() = default;

protected:

	virtual void _send   (const B *X_N1, const int frame_id);
};

}
}
#include "Sink_binary.hxx"

#endif /* SINK_BINARY_HPP */

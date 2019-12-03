/*!
 * \file
 * \brief
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef RADIO_USER_BINARY_HPP
#define RADIO_USER_BINARY_HPP

#include <thread>

#include "Module/Radio/Radio.hpp"

namespace aff3ct
{
namespace module
{
/*!
 * \class Radio
 *
 * \brief Transmit or receive data to or from a radio module.
 *
 * \tparam R: type of the data to send or receive.
 *
 */
template <typename R = double>
class Radio_user_binary : public Radio<R>
{
private:
	std::ifstream source_file;
	static std::thread::id master_thread_id;
public:
	/*!
	 * \brief Constructor.
	 *
	 * \param N:     Radio_user_binary frame length.
	 */
	Radio_user_binary(const int N, std::string filename, const int n_frames = 1);

	/*!
	 * \brief Destructor.
	 *
	 * \param N:     Radio_user_binary frame length.
	 */
	~Radio_user_binary() = default;
protected:
	void _send   (const R *X_N1, const int frame_id);
	void _receive(      R *Y_N1, const int frame_id);
};
}
}

#include "Radio_user_binary.hxx"

#endif /* RADIO_USER_BINARY_HPP */

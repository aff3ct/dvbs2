/*!
 * \file
 * \brief
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef RADIO_NO_HPP
#define RADIO_NO_HPP

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
class Radio_NO : public Radio<R>
{
public:
	/*!
	 * \brief Constructor.
	 *
	 * \param N:     Radio_NO frame length.
	 */
	Radio_NO(const int N, const int n_frames);

	/*!
	 * \brief Destructor.
	 *
	 * \param N:     Radio_NO frame length.
	 */
	~Radio_NO() = default;
protected:
	void _send   (const R *X_N1, const int frame_id);
	void _receive(      R *Y_N1, const int frame_id);
};
}
}

#endif /* RADIO_NO_HPP */

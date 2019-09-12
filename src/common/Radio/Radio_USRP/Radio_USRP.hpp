/*!
 * \file
 * \brief Pushes data to a file.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef RADIO_USRP_HPP
#define RADIO_USRP_HPP

#include "../Radio.hpp"


namespace aff3ct
{
namespace module
{
/*!
 * \class Radio
 *
 * \brief Transmit or receive data to or from a radio module.
 *
 * \tparam D: type of the data to send or receive.
 *
 */
template <typename D = float>
class Radio_USRP : public Radio<D>
{
public:
	/*!
	 * \brief Constructor.
	 *
	 * \param N:     Radio_USRP frame length.
	 */
	Radio_USRP(const int N, const int n_frames = 1) : Radio<D>(N, n_frames) {};

protected:
	void _send   (D *X_N1, const int frame_id) {std::cout << "Salut" << std::endl;}
	void _receive(D *Y_N1, const int frame_id) {std::cout << "Salut" << std::endl;}
};
}
}

#endif /* RADIO_USRP_HPP */

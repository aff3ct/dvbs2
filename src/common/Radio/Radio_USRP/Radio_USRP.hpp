/*!
 * \file
 * \brief Pushes data to a file.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef RADIO_USRP_HPP
#define RADIO_USRP_HPP

#include <uhd/usrp/multi_usrp.hpp>
#include <uhd.h>

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
private:
	uhd::usrp::multi_usrp::sptr usrp;

public:
	/*!
	 * \brief Constructor.
	 *
	 * \param N:     Radio_USRP frame length.
	 */
	Radio_USRP(const int N, const int n_frames = 1) ;

	/*!
	 * \brief Destructor.
	 *
	 * \param N:     Radio_USRP frame length.
	 */
	~Radio_USRP();
protected:
	void _send   (D *X_N1, const int frame_id);
	void _receive(D *Y_N1, const int frame_id);
};
}
}

#include "Radio_USRP.hxx"

#endif /* RADIO_USRP_HPP */

/*!
 * \file
 * \brief Pushes data to a file.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef RADIO_USRP_HPP
#define RADIO_USRP_HPP

#include <uhd.h>
#include <uhd/usrp/multi_usrp.hpp>

#include "Module/Radio/Radio.hpp"
#include "Factory/Module/Radio/Radio.hpp"

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
class Radio_USRP : public Radio<R>
{
private:
	uhd::usrp::multi_usrp::sptr usrp;
	uhd::stream_args_t          stream_args;
	uhd::rx_streamer::sptr      rx_stream;
	uhd::tx_streamer::sptr      tx_stream;

	boost::thread receive_thread;

	const bool threaded;
	const bool rx_enabled;
	const bool tx_enabled;

	std::vector<R*> fifo;

	std::atomic<bool> end;

	std::atomic<std::uint64_t> idx_w;
	std::atomic<std::uint64_t> idx_r;

	bool first_time;

public:
	/*!
	 * \brief Constructor.
	 *
	 * \param N:     Radio_USRP frame length.
	 */
	Radio_USRP(const factory::Radio& params, const int n_frames = 1);

	/*!
	 * \brief Destructor.
	 *
	 * \param N:     Radio_USRP frame length.
	 */
	~Radio_USRP();

protected:
	void _send   (const R *X_N1, const int frame_id);
	void _receive(      R *Y_N1, const int frame_id);

private:
	void thread_function();
	void receive_usrp(R *Y_N1);
	void fifo_read (R* Y_N1);
	void fifo_write(const std::vector<R>& tmp);
};
}
}

#include "Radio_USRP.hxx"

#endif /* RADIO_USRP_HPP */

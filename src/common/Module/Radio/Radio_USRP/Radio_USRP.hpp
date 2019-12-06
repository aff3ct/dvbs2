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
class Radio_USRP : public Radio<R>
{
private:
	uhd::usrp::multi_usrp::sptr usrp;
	uhd::stream_args_t          stream_args;
	uhd::rx_streamer::sptr      rx_stream;
	uhd::tx_streamer::sptr      tx_stream;

	std::thread receive_thread;

	const bool threaded;

	std::vector<std::unique_ptr<R[]>> fifo;
	R * array;
	std::atomic<std::uint64_t> idx_w;
	std::atomic<std::uint64_t> idx_r;

public:
	/*!
	 * \brief Constructor.
	 *
	 * \param N:     Radio_USRP frame length.
	 */
	Radio_USRP(const int N, std::string usrp_addr, const double clk_rate, const double rx_rate,
	           const double rx_freq, const std::string rx_subdev_spec, const std::string rx_antenna,
	           const double tx_rate, const double tx_freq, const std::string tx_subdev_spec,
	           const std::string tx_antenna, const double rx_gain, const double tx_gain, const bool threaded = false,
	           const uint64_t fifo_bytes = 0, const int n_frames = 1);

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
};
}
}

#include "Radio_USRP.hxx"

#endif /* RADIO_USRP_HPP */

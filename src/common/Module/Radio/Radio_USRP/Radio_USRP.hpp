#ifdef DVBS2_LINK_UHD

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
class Radio_USRP : public Radio<R>, public tools::Interface_waiting
{
private:
	uhd::usrp::multi_usrp::sptr usrp;
	uhd::stream_args_t          stream_args;
	uhd::rx_streamer::sptr      rx_stream;
	uhd::tx_streamer::sptr      tx_stream;

	std::thread send_thread;
	std::thread receive_thread;

	const bool threaded;
	const bool rx_enabled;
	const bool tx_enabled;

	std::vector<R*>       fifo_send;
	std::vector<R*>       fifo_receive;
	std::vector<int32_t*> fifo_ovf_flags;
	std::vector<int32_t*> fifo_seq_flags;
	std::vector<int32_t*> fifo_clt_flags;
	std::vector<int32_t*> fifo_tim_flags;

	std::atomic<bool> stop_threads;

	std::atomic<std::uint64_t> idx_w_send;
	std::atomic<std::uint64_t> idx_r_send;

	std::atomic<std::uint64_t> idx_w_receive;
	std::atomic<std::uint64_t> idx_r_receive;

	bool start_thread_send;
	bool start_thread_receive;

public:
	Radio_USRP(const factory::Radio& params, const int n_frames = 1);
	~Radio_USRP();

	void flush();
	virtual void reset();
	virtual void send_cancel_signal();
	virtual void wake_up();
	virtual void cancel_waiting();

protected:
	void _send   (const R *X_N1, const int frame_id);
	void _receive(      R *Y_N1, const int frame_id);

private:
	void thread_function_receive();
	inline void receive_usrp     (int32_t *OVF, int32_t *SEQ, int32_t *CLT, int32_t *TIM, R *Y_N1);
	inline void fifo_receive_read(int32_t *OVF, int32_t *SEQ, int32_t *CLT, int32_t *TIM, R *Y_N1);
	inline void fifo_receive_write();

	void thread_function_send();
	inline void send_usrp(const R *X_N1);
	inline void fifo_send_read();
	inline void fifo_send_write(const R* X_N1);
};
}
}

#endif /* RADIO_USRP_HPP */

#endif
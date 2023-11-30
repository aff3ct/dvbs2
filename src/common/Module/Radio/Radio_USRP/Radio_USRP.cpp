#ifdef DVBS2_LINK_UHD

#include <typeinfo>
#include <uhd/utils/thread.hpp>

#include "Tools/Thread_pinning/Thread_pinning.hpp"
#include "Module/Radio/Radio_USRP/Radio_USRP.hpp"

using namespace aff3ct;
using namespace aff3ct::module;

template <typename R>
Radio_USRP<R>
::Radio_USRP(const factory::Radio& params, const int n_frames)
: Radio<R>(params.N, params.n_frames),
  threaded(params.threaded),
  rx_enabled(params.rx_enabled),
  tx_enabled(params.tx_enabled),
  fifo_send     (threaded && tx_enabled ? uint64_t(1) + std::max(uint64_t(1), params.fifo_size / (2 * params.N * sizeof(R))) : 0),
  fifo_receive  (threaded && rx_enabled ? uint64_t(1) + std::max(uint64_t(1), params.fifo_size / (2 * params.N * sizeof(R))) : 0),
  fifo_ovf_flags(threaded && rx_enabled ? uint64_t(1) + std::max(uint64_t(1), params.fifo_size / (2 * params.N * sizeof(R))) : 0),
  fifo_seq_flags(threaded && rx_enabled ? uint64_t(1) + std::max(uint64_t(1), params.fifo_size / (2 * params.N * sizeof(R))) : 0),
  fifo_clt_flags(threaded && rx_enabled ? uint64_t(1) + std::max(uint64_t(1), params.fifo_size / (2 * params.N * sizeof(R))) : 0),
  fifo_tim_flags(threaded && rx_enabled ? uint64_t(1) + std::max(uint64_t(1), params.fifo_size / (2 * params.N * sizeof(R))) : 0),
  stop_threads(false),
  idx_w_send(0),
  idx_r_send(0),
  idx_w_receive(0),
  idx_r_receive(0),
  start_thread_send(false),
  start_thread_receive(false)
{
	const std::string name = "Radio_USRP";
	this->set_name(name);

	for (size_t i = 0; i < fifo_send.size(); i++)
		fifo_send[i] = new R[2 * params.N];
	for (size_t i = 0; i < fifo_receive.size(); i++)
		fifo_receive[i] = new R[2 * params.N];
	for (size_t i = 0; i < fifo_ovf_flags.size(); i++)
		fifo_ovf_flags[i] = new int32_t[1];
	for (size_t i = 0; i < fifo_seq_flags.size(); i++)
		fifo_seq_flags[i] = new int32_t[1];
	for (size_t i = 0; i < fifo_clt_flags.size(); i++)
		fifo_clt_flags[i] = new int32_t[1];
	for (size_t i = 0; i < fifo_tim_flags.size(); i++)
		fifo_tim_flags[i] = new int32_t[1];

	if (typeid(R) == typeid(R_8)  ||
	    typeid(R) == typeid(R_16) ||
	    typeid(R) == typeid(R_32) ||
	    typeid(R) == typeid(Q_32)  )
	{
		stream_args = uhd::stream_args_t("fc32");
	}
	else if (typeid(R) == typeid(R_64) ||
	         typeid(R) == typeid(Q_64)  )
	{
		stream_args = uhd::stream_args_t("fc64");
	}
	else if (typeid(R) == typeid(Q_16))
	{
		stream_args = uhd::stream_args_t("sc16");
	}
	else if (typeid(R) == typeid(Q_8))
	{
		stream_args = uhd::stream_args_t("sc8");
	}
	else
	{
		throw::runtime_error(__FILE__, __LINE__, __func__,
		                     "This data type (" + std::string(typeid(R).name()) + ") is not supported.");
	}

	// uhd::log::set_console_level(uhd::log::severity_level(3));
	// uhd::log::set_file_level   (uhd::log::severity_level(2));

	usrp = uhd::usrp::multi_usrp::make("addr=" + params.usrp_addr + ",master_clock_rate=" + std::to_string(params.clk_rate));
	usrp->set_master_clock_rate(params.clk_rate);

	if (params.rx_enabled)
	{
		usrp->set_rx_subdev_spec(uhd::usrp::subdev_spec_t(params.rx_subdev_spec));
		usrp->set_rx_antenna(params.rx_antenna);
		usrp->set_rx_freq(params.rx_freq);
		usrp->set_rx_gain(params.rx_gain);
		rx_stream = usrp->get_rx_stream(stream_args);
		usrp->set_rx_rate(params.rx_rate);
	}

	if (params.tx_enabled)
	{
		usrp->set_tx_subdev_spec(uhd::usrp::subdev_spec_t(params.tx_subdev_spec));
		usrp->set_tx_freq(params.tx_freq);
		usrp->set_tx_gain(params.tx_gain);
		usrp->set_tx_antenna(params.tx_antenna);
		tx_stream = usrp->get_tx_stream(stream_args);
		usrp->set_tx_rate(params.tx_rate);
	}

	if (!threaded)
	{
		usrp->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
	}
}

template <typename R>
Radio_USRP<R>
::~Radio_USRP()
{
	stop_threads = true;
	send_thread.join();
	receive_thread.join();
	usrp->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
	for (auto i = 0u; i < fifo_send.size(); i++)
		delete[] fifo_send[i];
	for (auto i = 0u; i < fifo_receive.size(); i++)
		delete[] fifo_receive[i];
	for (auto i = 0u; i < fifo_ovf_flags.size(); i++)
		delete[] fifo_ovf_flags[i];
	for (auto i = 0u; i < fifo_seq_flags.size(); i++)
		delete[] fifo_seq_flags[i];
	for (auto i = 0u; i < fifo_clt_flags.size(); i++)
		delete[] fifo_clt_flags[i];
	for (auto i = 0u; i < fifo_tim_flags.size(); i++)
		delete[] fifo_tim_flags[i];
}

template <typename R>
void Radio_USRP<R>
::_send(const R *X_N1, const int frame_id)
{
	if (!this->tx_enabled)
	{
		std::stringstream message;
		message << "send has been called while tx_rate has not been set.";
		throw tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (this->threaded && !this->start_thread_send)
	{
		this->start_thread_send = true;
		this->send_thread = std::thread(&Radio_USRP::thread_function_send, this);
	}

	if (threaded)
		fifo_send_write(X_N1);
	else
		send_usrp(X_N1);
}

template <typename R>
void Radio_USRP<R>
::_receive(R *Y_N1, const int frame_id)
{
	if (!this->rx_enabled)
	{
		std::stringstream message;
		message << "receive has been called while rx_rate has not been set.";
		throw tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (this->threaded && !this->start_thread_receive)
	{
		this->start_thread_receive = true;
		this->receive_thread = std::thread(&Radio_USRP::thread_function_receive, this);
	}

	if (threaded)
		fifo_receive_read(&this->ovf_flags[frame_id],
		                  &this->seq_flags[frame_id],
		                  &this->clt_flags[frame_id],
		                  &this->tim_flags[frame_id],
		                  Y_N1);
	else
		receive_usrp(&this->ovf_flags[frame_id],
		             &this->seq_flags[frame_id],
		             &this->clt_flags[frame_id],
		             &this->tim_flags[frame_id],
		             Y_N1);
}

template <typename R>
void Radio_USRP<R>
::fifo_send_read()
{
	bool has_read = false;
	while (!has_read && !stop_threads)
	{
		if (this->idx_w_send != this->idx_r_send)
		{
			send_usrp(fifo_send[this->idx_r_send]);
			this->idx_r_send = (this->idx_r_send +1) % fifo_send.size();
			has_read = true;
		}
	}
}

template <typename R>
void Radio_USRP<R>
::fifo_send_write(const R *X_N1)
{
	bool has_written = false;
	while (!has_written && !stop_threads)
	{
		if (((this->idx_w_send +1) % fifo_send.size()) != this->idx_r_send)
		{
			std::copy(X_N1, X_N1 + 2 * this->N, fifo_send[this->idx_w_send]);
			this->idx_w_send = (this->idx_w_send +1) % fifo_send.size();
			has_written = true;
		}
	}
}

template <typename R>
void Radio_USRP<R>
::fifo_receive_read(int32_t *OVF, int32_t *SEQ, int32_t *CLT, int32_t *TIM, R *Y_N1)
{
	bool has_read = false;
	while (!has_read && !stop_threads)
	{
		if (this->idx_w_receive != this->idx_r_receive)
		{
			std::copy(fifo_receive[this->idx_r_receive], fifo_receive[this->idx_r_receive] + 2 * this->N, Y_N1);
			*OVF = *(fifo_ovf_flags[this->idx_r_receive]);
			*SEQ = *(fifo_seq_flags[this->idx_r_receive]);
			*CLT = *(fifo_clt_flags[this->idx_r_receive]);
			*TIM = *(fifo_tim_flags[this->idx_r_receive]);
			this->idx_r_receive = (this->idx_r_receive +1) % fifo_receive.size();
			has_read = true;
		}
	}
}

template <typename R>
void Radio_USRP<R>
::fifo_receive_write()
{
	bool has_written = false;

	while (!has_written && !stop_threads)
	{
		if (((this->idx_w_receive +1) % fifo_receive.size()) != this->idx_r_receive)
		{
			receive_usrp(fifo_ovf_flags[this->idx_w_receive],
			             fifo_seq_flags[this->idx_w_receive],
			             fifo_clt_flags[this->idx_w_receive],
			             fifo_tim_flags[this->idx_w_receive],
			             fifo_receive  [this->idx_w_receive]);
			this->idx_w_receive = (this->idx_w_receive +1) % fifo_receive.size();
			has_written = true;
		}
	}
}

template <typename R>
void Radio_USRP<R>
::thread_function_send()
{
	aff3ct::tools::Thread_pinning::pin(3);

	uhd::set_thread_priority_safe();
	usrp->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);

	while (!stop_threads)
		this->fifo_send_read();

	aff3ct::tools::Thread_pinning::unpin();
}

template <typename R>
void Radio_USRP<R>
::thread_function_receive()
{
	aff3ct::tools::Thread_pinning::pin(1);

	uhd::set_thread_priority_safe();
	usrp->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);

	while (!stop_threads)
		this->fifo_receive_write();

	aff3ct::tools::Thread_pinning::unpin();
}

template <typename R>
void Radio_USRP<R>
::send_usrp(const R *X_N1)
{
	uhd::tx_metadata_t md;
	//md.start_of_burst = true;
	//md.end_of_burst   = false;
	//md.has_time_spec  = true;
	//md.time_spec      = usrp->get_time_now() + uhd::time_spec_t(0.1);
	tx_stream->send(X_N1, this->N, md);
}

template <typename R>
void Radio_USRP<R>
::receive_usrp(int32_t *OVF, int32_t *SEQ, int32_t *CLT, int32_t *TIM, R *Y_N1)
{
	uhd::rx_metadata_t md;

	auto num_rx_samps = 0;
	*OVF = 0;
	*SEQ = 0;
	*CLT = 0;
	*TIM = 0;
	while (num_rx_samps < this->N)
	{
		num_rx_samps += rx_stream->recv(Y_N1 + 2 * num_rx_samps, this->N - num_rx_samps, md);
		// handle the error codes
		switch (md.error_code)
		{
			case uhd::rx_metadata_t::ERROR_CODE_NONE:
				// possibility to log recovery after overflow
				break;

			// ERROR_CODE_OVERFLOW can indicate overflow or sequence error
			case uhd::rx_metadata_t::ERROR_CODE_OVERFLOW:
				// count overflows ?
				if (!md.out_of_sequence)
				{
					*OVF += 1;
					UHD_LOGGER_INFO("RADIO USRP") << "Detected overflow in Radio Rx.";
				} else
				{
					*SEQ += 1;
					UHD_LOGGER_INFO("RADIO USRP") << "Detected Rx sequence error.";
				}
				break;

			case uhd::rx_metadata_t::ERROR_CODE_LATE_COMMAND:
				*CLT += 1;
				UHD_LOGGER_ERROR("RADIO USRP") << "Receiver error: " << md.strerror();
				// Radio core will be in the idle state. Issue stream command to restart
				// streaming.
				// cmd.time_spec  = usrp->get_time_now() + uhd::time_spec_t(0.05);
				// rx_stream->issue_stream_cmd(cmd);
				break;

			case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT:
				*TIM += 1;
				UHD_LOGGER_ERROR("RADIO USRP") << "Receiver error: " << md.strerror();
				break;

				// Otherwise, it's an error
			default:
				// UHD_LOGGER_ERROR("RADIO USRP") << "Receiver error: " << md.strerror();
				throw::runtime_error(__FILE__, __LINE__, __func__, "Error in the Radio USRP streaming.");
				// std::cerr << "[" << "] Receiver error: " << md.strerror()
				//           << std::endl;
				// std::cerr << "[" << "] Unexpected error on recv, continuing..."
				//           << std::endl;
				break;
		}
	}
}

template <typename R>
void Radio_USRP<R>
::flush()
{
	this->idx_w_send = 0;
	this->idx_r_send = 0;
	this->idx_w_receive = 0;
	this->idx_r_receive = 0;
}

template <typename R>
void Radio_USRP<R>
::reset()
{
	this->stop_threads = true;
	this->send_thread.join();
	this->receive_thread.join();
	this->stop_threads = false;
	this->start_thread_send = false;
	this->start_thread_receive = false;
	this->flush();
}

template <typename R>
void Radio_USRP<R>
::send_cancel_signal()
{
	this->stop_threads = true;
}

template <typename R>
void Radio_USRP<R>
::wake_up()
{
}

template <typename R>
void Radio_USRP<R>
::cancel_waiting()
{
	this->send_cancel_signal();
	this->wake_up();
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Radio_USRP<double>;
template class aff3ct::module::Radio_USRP<float>;
template class aff3ct::module::Radio_USRP<int16_t>;
template class aff3ct::module::Radio_USRP<int8_t>;
// ==================================================================================== explicit template instantiation

#endif
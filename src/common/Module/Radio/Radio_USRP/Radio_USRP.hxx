#include <typeinfo>
#include <uhd/utils/thread.hpp>

namespace aff3ct
{
namespace module
{

template <typename R>
Radio_USRP<R>::
Radio_USRP(const factory::Radio& params, const int n_frames)
: Radio<R>(params.N, params.n_frames),
  threaded(params.threaded),
  rx_enabled(params.rx_enabled),
  tx_enabled(params.tx_enabled),
  fifo(uint64_t(1) + std::max(uint64_t(1), params.fifo_size / (2 * params.N * sizeof(R)))),
  end(false),
  idx_w(0),
  idx_r(0),
  first_time(true)
{
	for (size_t i = 0; i < fifo.size(); i++)
		fifo[i] = new R[2 * params.N];

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
Radio_USRP<R>::
~Radio_USRP()
{
	end = true;
	receive_thread.join();
	usrp->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
	for (auto i = 0u; i < fifo.size(); i++)
		delete[] fifo[i];
}

template <typename R>
void Radio_USRP<R>::
_send(const R *X_N1, const int frame_id)
{
	if (!this->tx_enabled)
	{
		std::stringstream message;
		message << "send has been called while tx_rate has not been set.";
		throw tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
	}
	uhd::tx_metadata_t md;
    md.start_of_burst = true;
    md.end_of_burst   = false;
    md.has_time_spec  = true;
    md.time_spec      = usrp->get_time_now() + uhd::time_spec_t(0.1);
	tx_stream->send(X_N1, this->N, md);
}

template <typename R>
void Radio_USRP<R>::
_receive(R *Y_N1, const int frame_id)
{
	if (!this->rx_enabled)
	{
		std::stringstream message;
		message << "receive has been called while rx_rate has not been set.";
		throw tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (this->threaded && this->first_time)
	{
		this->first_time = false;
		if (this->rx_enabled)
			this->receive_thread = boost::thread(&Radio_USRP::thread_function, this);
	}

	if (threaded)
		fifo_read(Y_N1);
	else
		receive_usrp(Y_N1);
}

template <typename R>
void Radio_USRP<R>::
fifo_read(R * Y_N1)
{
	bool has_read = false;
	while (!has_read)
	{
		if (this->idx_w != this->idx_r)
		{
			std::copy(fifo[this->idx_r], fifo[this->idx_r] + 2 * this->N, Y_N1);
			this->idx_r = (this->idx_r +1) % fifo.size();
			has_read = true;
		}
	}
}

template <typename R>
void Radio_USRP<R>::
fifo_write(const std::vector<R>& tmp)
{
	bool has_written = false;
	while (!has_written)
	{
		if (((this->idx_w +1) % fifo.size()) != this->idx_r)
		{
			receive_usrp(fifo[this->idx_w]);
			this->idx_w = (this->idx_w +1) % fifo.size();
			has_written = true;
		}
	}
}

template <typename R>
void Radio_USRP<R>::
thread_function()
{
    uhd::set_thread_priority_safe();
	std::vector<R> tmp(2 * this->N);
	usrp->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);

	while (!end)
	{
		bool has_written = false;
		while (!has_written)
		{
			if (((this->idx_w +1) % fifo.size()) != this->idx_r)
			{
				receive_usrp(fifo[this->idx_w]);
				this->idx_w = (this->idx_w +1) % fifo.size();
				has_written = true;
			}
		}
	}
}

template <typename R>
void Radio_USRP<R>::
receive_usrp(R *Y_N1)
{
	uhd::rx_metadata_t md;

	auto num_rx_samps = 0;
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
					UHD_LOGGER_INFO("RADIO USRP") << "Detected overflow in Radio Rx.";
				} else
				{
					UHD_LOGGER_INFO("RADIO USRP") << "Detected Rx sequence error.";
				}
				break;

			case uhd::rx_metadata_t::ERROR_CODE_LATE_COMMAND:
				UHD_LOGGER_ERROR("RADIO USRP") << "Receiver error: " << md.strerror();
				// Radio core will be in the idle state. Issue stream command to restart
				// streaming.
				// cmd.time_spec  = usrp->get_time_now() + uhd::time_spec_t(0.05);
				// rx_stream->issue_stream_cmd(cmd);
				break;

			case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT:
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

}
}

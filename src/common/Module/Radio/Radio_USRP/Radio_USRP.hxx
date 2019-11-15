#include <typeinfo>

namespace aff3ct
{
namespace module
{

template <typename R>
Radio_USRP<R>::
Radio_USRP(const int N, std::string usrp_addr, const double clk_rate, const double rx_rate,
           const double rx_freq, const std::string rx_subdev_spec, const std::string rx_antenna, const double tx_rate,
           const double tx_freq, const std::string tx_subdev_spec, const std::string tx_antenna, const int n_frames,
		   const double rx_gain, const double tx_gain)
: Radio<R>(N, n_frames)
{
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

	usrp = uhd::usrp::multi_usrp::make("addr=" + usrp_addr);
	usrp->set_master_clock_rate(clk_rate);

	usrp->set_rx_subdev_spec(uhd::usrp::subdev_spec_t(rx_subdev_spec));
	usrp->set_rx_antenna(rx_antenna);
	usrp->set_rx_rate(rx_rate);
	usrp->set_rx_freq(rx_freq);
	usrp->set_rx_gain(rx_gain);
	rx_stream = usrp->get_rx_stream(stream_args);

	usrp->set_tx_subdev_spec(uhd::usrp::subdev_spec_t(tx_subdev_spec));
	usrp->set_tx_rate(tx_rate);
	usrp->set_tx_freq(tx_freq);
	usrp->set_tx_gain(tx_gain);
	usrp->set_tx_antenna(tx_antenna);
	tx_stream = usrp->get_tx_stream(stream_args);

	usrp->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
}

template <typename R>
Radio_USRP<R>::
~Radio_USRP()
{
	usrp->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
}

template <typename R>
void Radio_USRP<R>::
_send(const R *X_N1, const int frame_id)
{
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
	std::vector<std::complex<R>> buff(this->N);
	uhd::rx_metadata_t md;

	auto num_rx_samps = 0;
	while (num_rx_samps < this->N)
	{
		num_rx_samps += rx_stream->recv(Y_N1 + num_rx_samps, this->N - num_rx_samps, md);

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
		        // cmd.stream_now = (buffs.size() == 1);
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

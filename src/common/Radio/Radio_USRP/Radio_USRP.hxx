#include "Radio_USRP.hpp"

namespace aff3ct
{
namespace module
{

template <typename D>
Radio_USRP<D>::
Radio_USRP(const int N, std::string usrp_addr, const double clk_rate, const double rx_rate,
           const double rx_freq, const std::string rx_subdev_spec, const double tx_rate,
           const double tx_freq, const std::string tx_subdev_spec, const int n_frames)
: Radio<D>(N, n_frames), stream_args("fc64")
{
	uhd::log::set_console_level(uhd::log::severity_level(3));
	uhd::log::set_file_level   (uhd::log::severity_level(2));

	usrp = uhd::usrp::multi_usrp::make("addr=" + usrp_addr);

	usrp->set_master_clock_rate(clk_rate);

	usrp->set_rx_rate(rx_rate);
	usrp->set_rx_freq(rx_freq);
	usrp->set_rx_subdev_spec(uhd::usrp::subdev_spec_t(rx_subdev_spec));

	usrp->set_rx_rate(tx_rate);
	usrp->set_rx_freq(tx_freq);
	usrp->set_rx_subdev_spec(uhd::usrp::subdev_spec_t(tx_subdev_spec));

	rx_stream = usrp->get_rx_stream(stream_args);

	usrp->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
}

template <typename D>
Radio_USRP<D>::
~Radio_USRP()
{
	usrp->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
}

template <typename D>
void Radio_USRP<D>::
_send(D *X_N1, const int frame_id)
{
}

template <typename D>
void Radio_USRP<D>::
_receive(D *Y_N1, const int frame_id)
{
	std::vector<std::complex<double>> buff(this->N);
	uhd::rx_metadata_t md;

	auto num_rx_samps = 0;
	while (num_rx_samps < this->N)
		num_rx_samps += rx_stream->recv(Y_N1 + num_rx_samps, this->N - num_rx_samps, md);
}

}
}

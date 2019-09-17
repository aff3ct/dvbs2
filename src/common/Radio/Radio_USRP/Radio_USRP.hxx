#include "Radio_USRP.hpp"

namespace aff3ct
{
namespace module
{

template <typename D>
Radio_USRP<D>::
Radio_USRP(const int N, std::string usrp_addr, const double rx_rate, const double rx_freq,
           const std::string rx_subdev_spec,   const double tx_rate, const double tx_freq,
           const std::string tx_subdev_spec, const int n_frames)
: Radio<D>(N, n_frames), stream_args("fc64")
{
	uhd::log::set_console_level(uhd::log::severity_level(3));
	uhd::log::set_file_level   (uhd::log::severity_level(2));

	usrp = uhd::usrp::multi_usrp::make("addr=" + usrp_addr); // Initialisation de l'USRP

	usrp->set_rx_rate(rx_rate);  // Set de la fréquence d'échantillonnage
	usrp->set_rx_freq(rx_freq);  // Set de la fréquence porteuse
	usrp->set_rx_subdev_spec(uhd::usrp::subdev_spec_t(rx_subdev_spec));

	usrp->set_rx_rate(tx_rate);
	usrp->set_rx_freq(tx_freq);
	usrp->set_rx_subdev_spec(uhd::usrp::subdev_spec_t(tx_subdev_spec));

	rx_stream = usrp->get_rx_stream(stream_args); // Pointeur sur les data reçues

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
	auto Ne = 16000;

	std::vector<std::complex<double>> buff(Ne); // Notre buffer à nous dans le programme
	uhd::rx_metadata_t md;                     // Des metadata

	auto num_rx_samps = 0;                     // Nombre d'echantillons reçus
	while (num_rx_samps < 16000)
	{
		num_rx_samps += rx_stream->recv(Y_N1 + num_rx_samps, Ne - num_rx_samps, md);
	}
}

}
}

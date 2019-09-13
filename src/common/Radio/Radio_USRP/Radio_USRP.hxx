#include "Radio_USRP.hpp"

namespace aff3ct
{
namespace module
{

template <typename D>
Radio_USRP<D>::
Radio_USRP(const int N, const int n_frames)
: Radio<D>(N, n_frames), stream_args("fc64")
{
	auto fc = 1090e6;
	auto fe = 8e6;

	std::string usrp_addr("addr=192.168.20.2");           // L'adresse de l'USRP est écrite en dur pour l'instant

	usrp = uhd::usrp::multi_usrp::make(usrp_addr); // Initialisation de l'USRP

	usrp->set_rx_rate(fe);                         // Set de la fréquence d'échantillonnage
	usrp->set_rx_freq(fc);                         // Set de la fréquence porteuse
	usrp->set_rx_subdev_spec(uhd::usrp::subdev_spec_t("A:0"));

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
		std::cout << num_rx_samps << std::endl;
	}
}

}
}

#include "Radio_USRP.hpp"

namespace aff3ct
{
namespace module
{

template <typename D>
Radio_USRP<D>::
Radio_USRP(const int N, const int n_frames)
: Radio<D>(N, n_frames)
{
	auto fc = 1090e6;
	auto fe = 8e6;

	//uhd::msg::register_handler(&my_handler);
	std::string usrp_addr("addr=192.168.222.145"); // L'adresse de l'USRP est écrite en dur pour l'instant

	usrp = uhd::usrp::multi_usrp::make(usrp_addr); // Initialisation de l'USRP

	usrp->set_rx_rate(fe);                         // Set de la fréquence d'échantillonnage
	usrp->set_rx_freq(fc);                         // Set de la fréquence porteuse
	usrp->set_rx_subdev_spec(uhd::usrp::subdev_spec_t("A:0"));
	// usrp->set_rx_antenna("RX2");


	std::cout << "# " << std::string(50, '-') << std::endl;
	std::cout << "# Let's get started" << std::endl;
	std::cout << "# " << std::string(50, '-') << std::endl;

	usrp->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);

	std::cout << "# Sampling Rate set to: " << usrp->get_rx_rate() << std::endl;
	std::cout << "# Central Frequency set to: " << usrp->get_rx_freq() << std::endl;
	std::cout << "# " << std::string(50, '-') << std::endl;
	std::cout << "# Ecoute en cours ..." << std::endl;

}

template <typename D>
Radio_USRP<D>::
~Radio_USRP()
{
	usrp->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);

	std::cout << "Au revoir" << std::endl;
}

template <typename D>
void Radio_USRP<D>::
_send(D *X_N1, const int frame_id)
{
	std::cout << "beng" << std::endl;
}

template <typename D>
void Radio_USRP<D>::
_receive(D *Y_N1, const int frame_id)
{
	std::cout << "beng" << std::endl;
}

}
}

// 	uhd::stream_args_t
// 		stream_args("fc64");        // Type des données à échantillonner (ici complexes float 64 - 32 bits par voie)
// 	uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args); // Pointeur sur les data reçues


// 	usrp->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
// 	rx_stream.reset();
// 	usrp.reset();

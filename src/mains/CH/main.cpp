// An exemple of use of this binary:
// ./bin/dvbs2_ch --rad-type USER_BIN --rad-rx-file-path after_TX.bin --rad-tx-file-path before_RX_4.5dB.bin --rad-rx-no-loop -m 4.5

#include <vector>
#include <numeric>
#include <string>
#include <iostream>

#include <aff3ct.hpp>

#include "Factory/DVBS2/DVBS2.hpp"

using namespace aff3ct;
using namespace aff3ct::module;

// global parameters
const std::string extension = "bin";

// aliases
template<class T> using uptr = std::unique_ptr<T>;

int main(int argc, char** argv)
{
	// setup signal handlers
	tools::Signal_handler::init();

	// get the parameter to configure the tools and modules
	const auto params = factory::DVBS2(argc, argv);

	// construct tools
	tools::Gaussian_noise_generator_fast<R> gen;
	std::vector<float> sigma(params.n_frames);

	// Eb/N0 en dB
	float ebn0 = params.ebn0_min;

	// compute the code rate
	const float R = (float)params.K_bch / (float)params.N_ldpc;
	// compute the current sigma for the channel noise
	const auto esn0  = tools::ebn0_to_esn0(ebn0, R, params.bps);
	std::fill(sigma.begin(), sigma.end(), tools::esn0_to_sigma(esn0));

	// construct base modules
	uptr<Radio<>                     > radio_rcv   (factory::DVBS2::build_radio              <>(params     ));
	uptr<Channel<>                   > channel     (factory::DVBS2::build_channel            <>(params, gen));
	uptr<Radio<>                     > radio_snd   (factory::DVBS2::build_radio              <>(params     ));
	uptr<Multiplier_fading_DVBS2<>   > fad_mlt     (factory::DVBS2::build_fading_mult        <>(params     ));
	uptr<Filter_Farrow_ccr_naive<>   > chn_frac_del(factory::DVBS2::build_channel_frac_delay <>(params     ));
	uptr<Variable_delay_cc_naive<>   > chn_int_del (factory::DVBS2::build_channel_int_delay  <>(params     ));
	uptr<Filter_buffered_delay<float>> chn_frm_del (factory::DVBS2::build_channel_frame_delay<>(params     ));
	uptr<Multiplier_sine_ccc_naive<> > freq_shift  (factory::DVBS2::build_freq_shift         <>(params     ));

	(*channel)[chn::sck::add_noise::CP] = sigma;

	if (params.channel_type == "SYNCHRO")
	{
		std::cout << "Channel AWGN+synchro" << std::endl; 
		(*fad_mlt     )[mlt::sck::imultiply::X_N ] = (*radio_rcv   )[rad::sck::receive  ::Y_N1];
		(*chn_frm_del )[flt::sck::filter   ::X_N1] = (*fad_mlt     )[mlt::sck::imultiply::Z_N ];
		(*chn_int_del )[flt::sck::filter   ::X_N1] = (*chn_frm_del )[flt::sck::filter   ::Y_N2];
		(*chn_frac_del)[flt::sck::filter   ::X_N1] = (*chn_int_del )[flt::sck::filter   ::Y_N2];
		(*freq_shift  )[mlt::sck::imultiply::X_N ] = (*chn_frac_del)[flt::sck::filter   ::Y_N2];
		(*channel     )[chn::sck::add_noise::X_N ] = (*freq_shift  )[mlt::sck::imultiply::Z_N ];
		(*radio_snd   )[rad::sck::send     ::X_N1] = (*channel     )[chn::sck::add_noise::Y_N ];
	}
	else
	{
		std::cout << "Channel AWGN" << std::endl; 
		(*channel  )[chn::sck::add_noise::X_N ] = (*radio_rcv)[rad::sck::receive  ::Y_N1];
		(*radio_snd)[rad::sck::send     ::X_N1] = (*channel  )[chn::sck::add_noise::Y_N ];
	}
	
	runtime::Sequence sequence_channel((*radio_rcv)[rad::tsk::receive]);

	uint64_t bytes = 0;
	sequence_channel.exec([&bytes, &params]()
	{
		bytes += 2 * params.p_rad.N * params.n_frames * sizeof(R);
		std::cout << "Samples size: " << (bytes / (1024 * 1024)) << " MB" << "\r";

		return false;
	});
	std::cout << "Samples size: " << (bytes / (1024 * 1024)) << " MB       " << std::endl;
	std::cout << rang::tag::info << "The samples are being written in the '"
	          << params.p_rad.tx_filepath << "' file... " << std::endl;
	std::cout.flush();

	return EXIT_SUCCESS;
}
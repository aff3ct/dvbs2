/*!
 * \file
 * \brief Generates a message.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef FRAMER_HXX_
#define FRAMER_HXX_

#include <sstream>
#include <complex>

#include "Tools/Exception/exception.hpp"

#include "Framer.hpp"

namespace aff3ct
{
namespace module
{

template <typename B>
Framer<B>::
Framer(const int XFEC_FRAME_SIZE, const int PL_FRAME_SIZE, const std::string MODCOD, const int n_frames)
: Module(n_frames), XFEC_FRAME_SIZE(XFEC_FRAME_SIZE), PL_FRAME_SIZE(PL_FRAME_SIZE), MODCOD(MODCOD)
{
	const std::string name = "Framer";
	this->set_name(name);
	this->set_short_name(name);

	this->generate_PLH();
	
	this->M = 90;
	this->N_PILOTS = this->XFEC_FRAME_SIZE / (2*16*this->M);

	if (XFEC_FRAME_SIZE <= 0)
	{
		std::stringstream message;
		message << "'XFEC_FRAME_SIZE' has to be greater than 0 ('XFEC_FRAME_SIZE' = " << XFEC_FRAME_SIZE << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}
	if (PL_FRAME_SIZE <= 0)
	{
		std::stringstream message;
		message << "'PL_FRAME_SIZE' has to be greater than 0 ('PL_FRAME_SIZE' = " << PL_FRAME_SIZE << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	auto &p = this->create_task("generate");
	auto &ps_XFEC_frame = this->template create_socket_out<B>(p, "XFEC_frame", this->XFEC_FRAME_SIZE * this->n_frames);
	auto &ps_PL_frame = this->template create_socket_out<B>(p, "PL_frame", this->PL_FRAME_SIZE * this->n_frames);
	this->create_codelet(p, [this, &ps_PL_frame, &ps_XFEC_frame]() -> int
	{
		this->generate(static_cast<B*>(ps_PL_frame.get_dataptr()), static_cast<B*>(ps_XFEC_frame.get_dataptr()));

		return 0;
	});
}

/*template <typename B>
int Framer<B>::
get_K() const
{
	return K;
}*/

template <typename B>
void Framer<B>::
generate_PLH( void )
{
	this->PLH.resize(2*90);
	std::vector<std::vector<int> > G_32_7 { { 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1},
												{ 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1},
												{ 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1},
												{ 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1},
												{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
												{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
												{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1} };

	const std::vector<int> PLS_scrambler_sequence{0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0};
	
	std::vector <int > mod_cod(7);
	const std::vector <int > mod_cod_Q_8_9{0, 0, 1, 0, 1, 0, 1};
	const std::vector <int > mod_cod_Q_3_5{0, 0, 0, 1, 0, 1, 1};
	const std::vector <int > mod_cod_8_3_5{0, 0, 1, 1, 0, 0, 1};
	const std::vector <int > mod_cod_8_8_9{0, 1, 0, 0, 0, 0, 1};
	const std::vector <int > mod_cod_16_8_9{0, 1, 0, 1, 1, 0, 1};

	if(MODCOD == "QPSK-S_8/9")
		mod_cod = mod_cod_Q_8_9; // QPSK 8/9 : 21
	else if(MODCOD == "QPSK-S_3/5")
		mod_cod = mod_cod_Q_3_5; // QPSK 3/5 : 11
	else if(MODCOD == "8PSK-S_3/5")
		mod_cod = mod_cod_8_3_5; // 8PSK 3/5 : 25
	else if(MODCOD == "8PSK-S_8/9")
		mod_cod = mod_cod_8_8_9; // 8PSK 8/9 : 33
	else if(MODCOD == "16APSK-S_8/9")
		mod_cod = mod_cod_16_8_9; // 16APSK 8/9 : 45

	//const int pilot_insert = 1; // pilots are inserted
	//const int short_code = 1; // short LDPC frame is used
	std::vector <int > coded_PLS(32, 0);
	std::vector <int > complementary_coded_PLS(32, 0);
	std::vector <int > final_PLS(64, 0);

	
	// generate and modulate SOF
	const std::vector<int > SOF_bin {0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0};
	std::vector<int > SOF_bpsk(26);

	std::vector<float > SOF_mod(52);
	std::vector<std::complex<float> > Cx_SOF_mod(26);

	std::vector<int > final_PLS_bpsk(64);
	std::vector<float > final_PLS_mod(128);
	std::vector<std::complex<float> > Cx_final_PLS_mod(64);

	for( int i = 0; i < 26; i++)
		SOF_bpsk[i] = (1 - 2*SOF_bin[i]);

	for( int i = 0; i < 13; i++)
	{
		SOF_mod[4*i    ] =      (1/sqrt(2)) * SOF_bpsk[2*i]; // real part of the even symbols (starting index = 0)
		SOF_mod[4*i + 1] =      (1/sqrt(2)) * SOF_bpsk[2*i]; // imag part of the even symbols
		SOF_mod[4*i + 2] = -1 * (1/sqrt(2)) * SOF_bpsk[2*i+1]; // real part of the odd symbols
		SOF_mod[4*i + 3] =      (1/sqrt(2)) * SOF_bpsk[2*i+1]; // imag part of the odd symbols
	}

	for(int col=0; col<32 ; col++)
	{
		for(int row=0; row<7 ; row++)
		{
			coded_PLS[col] = (coded_PLS[col] + mod_cod[row] * G_32_7[row][col])%2;
			complementary_coded_PLS[col] = 	(coded_PLS[col] == 0) ? 1 : 0;
		}
	}

	// interleave PLS and complementary PLS (p.47 DVB-S2X)
	// scramble and the same time
	for(int i=0; i<32 ; i++)
	{
		final_PLS[2*i] = (coded_PLS[i] + PLS_scrambler_sequence[2*i])%2;
		final_PLS[2*i+1] = (complementary_coded_PLS[i] + PLS_scrambler_sequence[2*i+1])%2;
	}

	// Pi/2 BPSK modulation
	for( int i = 0; i < 64; i++)
		final_PLS_bpsk[i] = (1 - 2*final_PLS[i]);

	if(mod_cod[0] == 0)
		for( int i = 0; i < 32; i++)
		{
			final_PLS_mod[4*i    ] =      (1/sqrt(2)) * final_PLS_bpsk[2*i]; // real part of the even symbols (starting index = 0)
			final_PLS_mod[4*i + 1] =      (1/sqrt(2)) * final_PLS_bpsk[2*i]; // imag part of the even symbols
			final_PLS_mod[4*i + 2] = -1 * (1/sqrt(2)) * final_PLS_bpsk[2*i+1]; // real part of the odd symbols
			final_PLS_mod[4*i + 3] =      (1/sqrt(2)) * final_PLS_bpsk[2*i+1]; // imag part of the odd symbols
		}
	else // include pi/2 jump if b_0 == 1 (See note 2 p.44, DVBS2X)
		for( int i = 0; i < 32; i++)
		{
			final_PLS_mod[4*i    ] = -1 * (1/sqrt(2)) * final_PLS_bpsk[2*i]; // real part of the even symbols (starting index = 0)
			final_PLS_mod[4*i + 1] =      (1/sqrt(2)) * final_PLS_bpsk[2*i]; // imag part of the even symbols
			final_PLS_mod[4*i + 2] = -1 * (1/sqrt(2)) * final_PLS_bpsk[2*i+1]; // real part of the odd symbols
			final_PLS_mod[4*i + 3] = -1 * (1/sqrt(2)) * final_PLS_bpsk[2*i+1]; // imag part of the odd symbols
		}

	this->PLH = SOF_mod; // assign SOF
	this->PLH.insert( this->PLH.end(), final_PLS_mod.begin(), final_PLS_mod.end()); // append PLS

}

template <typename B>
template <class A>
void Framer<B>::
generate(std::vector<B,A>& XFEC_frame, std::vector<B,A>& PL_frame, const int frame_id)
{
	if (this->XFEC_FRAME_SIZE * this->n_frames != (int)XFEC_frame.size())
	{
		std::stringstream message;
		message << "'XFEC_frame.size()' has to be equal to 'XFEC_FRAME_SIZE' * 'n_frames' ('XFEC_frame.size()' = " << XFEC_frame.size()
		        << ", 'XFEC_FRAME_SIZE' = " << this->XFEC_FRAME_SIZE << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}
	if (this->PL_FRAME_SIZE * this->n_frames != (int)PL_frame.size())
	{
		std::stringstream message;
		message << "'PL_frame.size()' has to be equal to 'PL_FRAME_SIZE' * 'n_frames' ('PL_frame.size()' = " << PL_frame.size()
		        << ", 'PL_FRAME_SIZE' = " << this->PL_FRAME_SIZE << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	this->generate(XFEC_frame.data(), PL_frame.data(), frame_id);
}

template <typename B>
void Framer<B>::
generate(B *XFEC_frame, B *PL_frame, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_generate(XFEC_frame + f * this->XFEC_FRAME_SIZE, PL_frame + f * this->PL_FRAME_SIZE, f);
}
template <typename B>
void Framer<B>::
_generate(B *XFEC_frame, B *PL_frame, const int frame_id)
{

		std::vector <std::complex<float> > Cx_XFEC_frame(this->XFEC_FRAME_SIZE/2);
		
		for (unsigned int i = 0; i < Cx_XFEC_frame.size(); i++)
		{
			Cx_XFEC_frame[i].real(XFEC_frame[2*i]);
			Cx_XFEC_frame[i].imag(XFEC_frame[2*i+1]);
		}

		////////////////////////////////////////////////////
		// Pilot generation
		////////////////////////////////////////////////////

		std::vector <std::complex<float> > Cx_pilot_mod(36);//, (1/sqrt(2), 1/sqrt(2)) );
		
		for (unsigned int i = 0; i < Cx_pilot_mod.size(); i++)
		{
			Cx_pilot_mod[i].real(1/sqrt(2));
			Cx_pilot_mod[i].imag(1/sqrt(2));
		}

		////////////////////////////////////////////////////
		// Framing : PL_HEADER + DATA + PILOTS
		////////////////////////////////////////////////////

		std::vector<std::complex<float> > Cx_PL_FRAME(90);
		std::vector<std::complex<float> > Cx_PLH(90);
		for (unsigned int i = 0; i < Cx_PLH.size(); i++)
		{
			Cx_PLH[i].real(this->PLH[2*i]);
			Cx_PLH[i].imag(this->PLH[2*i+1]);
		}
		Cx_PL_FRAME = Cx_PLH;

		std::vector<std::complex<float> >::iterator Cx_data_it;
		Cx_data_it = Cx_XFEC_frame.begin();

		std::vector<std::complex<float> > Cx_data_slice(16*this->M);
		std::vector<std::complex<float> > Cx_data_remainder( this->XFEC_FRAME_SIZE/2 - (16*this->M*this->N_PILOTS) ); // remaining data at the end of the XFEC_frame

		for(int i = 0; i < this->N_PILOTS; i++)
		{
			Cx_data_slice.assign(Cx_data_it, Cx_data_it+(16*this->M));
			Cx_PL_FRAME.insert(Cx_PL_FRAME.end(), Cx_data_slice.begin(), Cx_data_slice.end()); // append 16 slots of data
			Cx_PL_FRAME.insert(Cx_PL_FRAME.end(), Cx_pilot_mod.begin(), Cx_pilot_mod.end()); // append the pilot
			Cx_data_it += (16*M); // update the data index to the next block of 16 slots			
		}

		Cx_data_remainder.assign(Cx_XFEC_frame.begin()+(16*M*N_PILOTS), Cx_XFEC_frame.end());

		Cx_PL_FRAME.insert(Cx_PL_FRAME.end(), Cx_data_remainder.begin(), Cx_data_remainder.end());

		for (unsigned int i = 0; i < Cx_PL_FRAME.size(); i++)
		{
			PL_frame[2*i] = Cx_PL_FRAME[i].real();
			PL_frame[2*i+1] = Cx_PL_FRAME[i].imag();
		}
}

}
}

#endif

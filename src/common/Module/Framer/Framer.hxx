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

#include "Module/Framer/Framer.hpp"

namespace aff3ct
{
namespace module
{

template <typename B>
Framer<B>::
Framer(const int xfec_frame_size, const int pl_frame_size, const std::string modcod, const int n_frames)
: Module(), xfec_frame_size(xfec_frame_size), pl_frame_size(pl_frame_size), modcod(modcod)
{
	const std::string name = "Framer";
	this->set_name(name);
	this->set_short_name(name);
	this->set_n_frames(n_frames);
	this->set_single_wave(true);

	this->generate_plh();

	this->M = 90;
	this->P = 36;
	this->N_pilots = this->xfec_frame_size / (2*16*this->M);

	if (xfec_frame_size <= 0)
	{
		std::stringstream message;
		message << "'xfec_frame_size' has to be greater than 0 ('xfec_frame_size' = " << xfec_frame_size << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}
	if (pl_frame_size <= 0)
	{
		std::stringstream message;
		message << "'pl_frame_size' has to be greater than 0 ('pl_frame_size' = " << pl_frame_size << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	auto &p1 = this->create_task("generate");
	auto p1s_Y_N1 = this->template create_socket_in <B>(p1, "Y_N1", this->xfec_frame_size);
	auto p1s_Y_N2 = this->template create_socket_out<B>(p1, "Y_N2", this->pl_frame_size  );
	this->create_codelet(p1, [p1s_Y_N1, p1s_Y_N2](Module &m, Task &t, const size_t frame_id) -> int
	{
		static_cast<Framer<B>&>(m).generate(static_cast<B*>(t[p1s_Y_N1].get_dataptr()),
		                                    static_cast<B*>(t[p1s_Y_N2].get_dataptr()));
		return 0;
	});

	auto &p2 = this->create_task("remove_plh");
	auto p2s_Y_N1 = this->template create_socket_in <B>(p2, "Y_N1", this->pl_frame_size  );
	auto p2s_Y_N2 = this->template create_socket_out<B>(p2, "Y_N2", this->xfec_frame_size);
	this->create_codelet(p2, [p2s_Y_N1, p2s_Y_N2](Module &m, Task &t, const size_t frame_id) -> int
	{
		static_cast<Framer<B>&>(m).remove_plh(static_cast<B*>(t[p2s_Y_N1].get_dataptr()),
		                                      static_cast<B*>(t[p2s_Y_N2].get_dataptr()));

		return 0;
	});
}

template <typename B>
Framer<B>* Framer<B>
::clone() const
{
	auto m = new Framer(*this);
	m->deep_copy(*this);
	return m;
}

/*template <typename B>
int Framer<B>::
get_K() const
{
	return K;
}*/

template <typename B>
void Framer<B>::
generate_plh( void )
{
	this->plh.resize(2*90);
	std::vector<std::vector<int> > G_32_7 { { 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1},
	                                        { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1},
	                                        { 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1},
	                                        { 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1},
	                                        { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
	                                        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	                                        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1} };

	const std::vector<int> PLS_scrambler_sequence{0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0};

	std::vector <int > mod_cod(7);
	const std::vector <int > mod_cod_Q_8_9 {0, 0, 1, 0, 1, 0, 1};
	const std::vector <int > mod_cod_Q_3_5 {0, 0, 0, 1, 0, 1, 1};
	const std::vector <int > mod_cod_8_3_5 {0, 0, 1, 1, 0, 0, 1};
	const std::vector <int > mod_cod_8_8_9 {0, 1, 0, 0, 0, 0, 1};
	const std::vector <int > mod_cod_16_8_9{0, 1, 0, 1, 1, 0, 1};

	if (modcod == "QPSK-S_8/9")
		mod_cod = mod_cod_Q_8_9; // QPSK 8/9 : 21
	else if (modcod == "QPSK-S_3/5")
		mod_cod = mod_cod_Q_3_5; // QPSK 3/5 : 11
	else if (modcod == "8PSK-S_3/5")
		mod_cod = mod_cod_8_3_5; // 8PSK 3/5 : 25
	else if (modcod == "8PSK-S_8/9")
		mod_cod = mod_cod_8_8_9; // 8PSK 8/9 : 33
	else if (modcod == "16APSK-S_8/9")
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

	std::vector<int > final_PLS_bpsk(64);
	std::vector<float > final_PLS_mod(128);

	for( int i = 0; i < 26; i++)
		SOF_bpsk[i] = (1 - 2*SOF_bin[i]);

	for( int i = 0; i < 13; i++)
	{
		SOF_mod[4*i    ] =      (1/std::sqrt(2)) * SOF_bpsk[2*i]; // real part of the even symbols (starting index = 0)
		SOF_mod[4*i + 1] =      (1/std::sqrt(2)) * SOF_bpsk[2*i]; // imag part of the even symbols
		SOF_mod[4*i + 2] = -1 * (1/std::sqrt(2)) * SOF_bpsk[2*i+1]; // real part of the odd symbols
		SOF_mod[4*i + 3] =      (1/std::sqrt(2)) * SOF_bpsk[2*i+1]; // imag part of the odd symbols
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

	if (mod_cod[0] == 0)
		for( int i = 0; i < 32; i++)
		{
			final_PLS_mod[4*i    ] =      (1/std::sqrt(2)) * final_PLS_bpsk[2*i]; // real part of the even symbols (starting index = 0)
			final_PLS_mod[4*i + 1] =      (1/std::sqrt(2)) * final_PLS_bpsk[2*i]; // imag part of the even symbols
			final_PLS_mod[4*i + 2] = -1 * (1/std::sqrt(2)) * final_PLS_bpsk[2*i+1]; // real part of the odd symbols
			final_PLS_mod[4*i + 3] =      (1/std::sqrt(2)) * final_PLS_bpsk[2*i+1]; // imag part of the odd symbols
		}
	else // include pi/2 jump if b_0 == 1 (See note 2 p.44, DVBS2X)
		for( int i = 0; i < 32; i++)
		{
			final_PLS_mod[4*i    ] = -1 * (1/std::sqrt(2)) * final_PLS_bpsk[2*i]; // real part of the even symbols (starting index = 0)
			final_PLS_mod[4*i + 1] =      (1/std::sqrt(2)) * final_PLS_bpsk[2*i]; // imag part of the even symbols
			final_PLS_mod[4*i + 2] = -1 * (1/std::sqrt(2)) * final_PLS_bpsk[2*i+1]; // real part of the odd symbols
			final_PLS_mod[4*i + 3] = -1 * (1/std::sqrt(2)) * final_PLS_bpsk[2*i+1]; // imag part of the odd symbols
		}

	this->plh = SOF_mod; // assign SOF
	this->plh.insert( this->plh.end(), final_PLS_mod.begin(), final_PLS_mod.end()); // append PLS

}

template <typename B>
template <class A>
void Framer<B>::
generate(std::vector<B,A>& Y_N1, std::vector<B,A>& Y_N2, const int frame_id)
{
	if (this->xfec_frame_size * this->n_frames != (int)Y_N1.size())
	{
		std::stringstream message;
		message << "'Y_N1.size()' has to be equal to 'xfec_frame_size' * 'n_frames' ('Y_N1.size()' = " << Y_N1.size()
		        << ", 'xfec_frame_size' = " << this->xfec_frame_size << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}
	if (this->pl_frame_size * this->n_frames != (int)Y_N2.size())
	{
		std::stringstream message;
		message << "'Y_N2.size()' has to be equal to 'pl_frame_size' * 'n_frames' ('Y_N2.size()' = " << Y_N2.size()
		        << ", 'pl_frame_size' = " << this->pl_frame_size << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	this->generate(Y_N1.data(), Y_N2.data(), frame_id);
}

template <typename B>
void Framer<B>::
generate(B *Y_N1, B *Y_N2, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_generate(Y_N1 + f * this->xfec_frame_size, Y_N2 + f * this->pl_frame_size, f);
}

template <typename B>
void Framer<B>::
_generate(B *Y_N1, B *Y_N2, const int frame_id)
{

	std::vector <std::complex<float> > Cx_XFEC_frame(this->xfec_frame_size/2);

	for (unsigned int i = 0; i < Cx_XFEC_frame.size(); i++)
	{
		Cx_XFEC_frame[i].real(Y_N1[2*i]);
		Cx_XFEC_frame[i].imag(Y_N1[2*i+1]);
	}

	////////////////////////////////////////////////////
	// Pilot generation
	////////////////////////////////////////////////////

	std::vector <std::complex<float> > Cx_pilot_mod(36);//, (1/std::sqrt(2), 1/std::sqrt(2)) );

	for (unsigned int i = 0; i < Cx_pilot_mod.size(); i++)
	{
		Cx_pilot_mod[i].real(1/std::sqrt(2));
		Cx_pilot_mod[i].imag(1/std::sqrt(2));
	}

	////////////////////////////////////////////////////
	// Framing : PL_HEADER + DATA + PILOTS
	////////////////////////////////////////////////////

	std::vector<std::complex<float> > Cx_PL_FRAME(90);
	std::vector<std::complex<float> > Cx_plh(90);
	for (unsigned int i = 0; i < Cx_plh.size(); i++)
	{
		Cx_plh[i].real(this->plh[2*i]);
		Cx_plh[i].imag(this->plh[2*i+1]);
	}
	Cx_PL_FRAME = Cx_plh;

	std::vector<std::complex<float> >::iterator Cx_data_it;
	Cx_data_it = Cx_XFEC_frame.begin();

	std::vector<std::complex<float> > Cx_data_slice(16*this->M);
	std::vector<std::complex<float> > Cx_data_remainder( this->xfec_frame_size/2 - (16*this->M*this->N_pilots) ); // remaining data at the end of the Y_N1

	for(int i = 0; i < this->N_pilots; i++)
	{
		Cx_data_slice.assign(Cx_data_it, Cx_data_it+(16*this->M));
		Cx_PL_FRAME.insert(Cx_PL_FRAME.end(), Cx_data_slice.begin(), Cx_data_slice.end()); // append 16 slots of data
		Cx_PL_FRAME.insert(Cx_PL_FRAME.end(), Cx_pilot_mod.begin(), Cx_pilot_mod.end()); // append the pilot
		Cx_data_it += (16*M); // update the data index to the next block of 16 slots
	}

	Cx_data_remainder.assign(Cx_XFEC_frame.begin()+(16*M*N_pilots), Cx_XFEC_frame.end());

	Cx_PL_FRAME.insert(Cx_PL_FRAME.end(), Cx_data_remainder.begin(), Cx_data_remainder.end());

	for (unsigned int i = 0; i < Cx_PL_FRAME.size(); i++)
	{
		Y_N2[2*i] = Cx_PL_FRAME[i].real();
		Y_N2[2*i+1] = Cx_PL_FRAME[i].imag();
	}
}


template <typename B>
template <class A>
void Framer<B>::
remove_plh(std::vector<B,A>& Y_N1, std::vector<B,A>& Y_N2, const int frame_id)
{
	if (this->xfec_frame_size * this->n_frames != (int)Y_N2.size())
	{
		std::stringstream message;
		message << "'Y_N2.size()' has to be equal to 'xfec_frame_size' * 'n_frames' ('Y_N2.size()' = " << Y_N2.size()
		        << ", 'xfec_frame_size' = " << this->xfec_frame_size << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}
	if (this->pl_frame_size * this->n_frames != (int)Y_N1.size())
	{
		std::stringstream message;
		message << "'PL_frame.size()' has to be equal to 'pl_frame_size' * 'n_frames' ('PL_frame.size()' = " << Y_N1.size()
		        << ", 'pl_frame_size' = " << this->pl_frame_size << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	this->remove_plh(Y_N1.data(), Y_N2.data(), frame_id);
}

template <typename B>
void Framer<B>::
remove_plh(B *Y_N1, B *Y_N2, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_remove_plh(Y_N1 + f * this->pl_frame_size, Y_N2 + f * this->xfec_frame_size, f);
}

template <typename B>
void Framer<B>::
_remove_plh(B *Y_N1, B *Y_N2, const int frame_id)
{
	std::vector<B> pl_frame_tmp(pl_frame_size);
	std::copy(Y_N1, Y_N1 + pl_frame_size, pl_frame_tmp.data());
	pl_frame_tmp.erase(pl_frame_tmp.begin(), pl_frame_tmp.begin() + 2 * this->M); // erase the pl header
	for( int i = 1; i < this->N_pilots +1; i++)
	{
		pl_frame_tmp.erase(pl_frame_tmp.begin()+(i * this->M * 16 * 2),
		                   pl_frame_tmp.begin()+(i * this->M * 16 * 2) + (this->P * 2));
	}
	std::copy(pl_frame_tmp.data(), pl_frame_tmp.data() + xfec_frame_size, Y_N2);
}

}
}

#endif

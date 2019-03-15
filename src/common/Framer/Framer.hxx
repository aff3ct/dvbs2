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

#include "Tools/Exception/exception.hpp"

#include "Framer.hpp"

namespace aff3ct
{
namespace module
{

template <typename B>
Framer<B>::
Framer(const int K, const int n_frames)
: Module(n_frames), K(K)
{
	const std::string name = "Framer";
	this->set_name(name);
	this->set_short_name(name);

	this->generate_PLH();

	N_LDPC = 16200;
	BPS = 2;
	N_XFEC_FRAME = N_LDPC / BPS;
	M = 90;
	N_PILOTS = N_XFEC_FRAME / (16*M);

	if (K <= 0)
	{
		std::stringstream message;
		message << "'K' has to be greater than 0 ('K' = " << K << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	auto &p = this->create_task("generate");
	auto &ps_XFEC_frame = this->template create_socket_out<B>(p, "XFEC_frame", this->N_XFEC_FRAME * this->n_frames);
	auto &ps_U_K = this->template create_socket_out<B>(p, "U_K", this->K * this->n_frames);
	this->create_codelet(p, [this, &ps_U_K, &ps_XFEC_frame]() -> int
	{
		this->generate(static_cast<B*>(ps_U_K.get_dataptr()), static_cast<B*>(ps_XFEC_frame.get_dataptr()));

		return 0;
	});
}

template <typename B>
int Framer<B>::
get_K() const
{
	return K;
}

template <typename B>
void Framer<B>::
generate_PLH( void )
{
	//for (auto i = 0; i < this->K; i++)
	//	U_K[i] = (B)this->uniform_dist(this->rd_engine);
	this->PLH.resize(2*90);
	std::vector<std::vector<int> > G_32_7 { { 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1},
												{ 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1},
												{ 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1},
												{ 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1},
												{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
												{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
												{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1} };

	const std::vector<int> PLS_scrambler_sequence{0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0};
	
	//const vector <int > {} // QPSK 3/5
	const std::vector <int > mod_cod{0, 0, 1, 0, 1, 0, 1}; // QPSK 8/9
	//const vector <int > {} // 8PSK 3/5
	//const vector <int > {} // 8PSK 8/9
	//const vector <int > {} // 16APSK 8/9
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

	//U_K.resize(2*26);
	this->PLH = SOF_mod; // assign SOF
	this->PLH.insert( this->PLH.end(), final_PLS_mod.begin(), final_PLS_mod.end()); // append PLS

	//U_K = &PLH[0]; // assign SOF	
	//std::copy(PLH.begin(), PLH.end(), U_K);
}

template <typename B>
template <class A>
void Framer<B>::
generate(std::vector<B,A>& U_K, std::vector<B> XFEC_frame, const int frame_id)
{
	if (this->K * this->n_frames != (int)U_K.size())
	{
		std::stringstream message;
		message << "'U_K.size()' has to be equal to 'K' * 'n_frames' ('U_K.size()' = " << U_K.size()
		        << ", 'K' = " << this->K << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	this->generate(U_K.data(), XFEC_frame.data(), frame_id);
}

template <typename B>
void Framer<B>::
generate(B *U_K, B *XFEC_frame, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_generate(U_K + f * this->K, XFEC_frame + f * this->N_XFEC_FRAME, f);
}
template <typename B>
void Framer<B>::
_generate(B *U_K, B *XFEC_frame, const int frame_id)
{

		std::vector <float > XFEC_frame_v;
		XFEC_frame_v.assign(XFEC_frame, XFEC_frame + 2*N_LDPC/BPS);
	
		////////////////////////////////////////////////////
		// Pilot generation
		////////////////////////////////////////////////////

		std::vector <float > pilot_mod(72, (1/sqrt(2)) );

		////////////////////////////////////////////////////
		// Framing : PL_HEADER + DATA + PILOTS
		////////////////////////////////////////////////////

		std::vector<float> PL_FRAME;

		PL_FRAME.resize(2*90);
		PL_FRAME = this->PLH;

		std::vector<float>::iterator data_it;
		data_it = XFEC_frame_v.begin();

		std::vector<float> data_slice((2*16*M)); // 16 slots of data
		std::vector<float> data_remainder( 2*(N_XFEC_FRAME - (16*M*N_PILOTS))); // remaining data at the end of the XFEC_frame

		for(int i = 0; i < N_PILOTS; i++)
		{
			data_slice.assign(data_it, data_it+(2*16*M));
			PL_FRAME.insert(PL_FRAME.end(), data_slice.begin(), data_slice.end()); // append 16 slots of data
			PL_FRAME.insert(PL_FRAME.end(), pilot_mod.begin(), pilot_mod.end()); // append the pilot
			data_it += (2*16*M); // update the data index to the next block of 16 slots			
		}

		data_remainder.assign(XFEC_frame_v.begin()+(2*16*M*N_PILOTS), XFEC_frame_v.end());

		PL_FRAME.insert(PL_FRAME.end(), data_remainder.begin(), data_remainder.end());
		std::copy(PL_FRAME.begin(), PL_FRAME.end(), U_K);



}

}
}

#endif

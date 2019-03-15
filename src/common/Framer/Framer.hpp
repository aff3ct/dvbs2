/*!
 * \file
 * \brief Generates a message.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef FRAMER_HPP_
#define FRAMER_HPP_

#include <vector>
#include <string>
#include <iostream>

#include "Module/Module.hpp"

namespace aff3ct
{
namespace module
{
	namespace frm
	{
		enum class tsk : uint8_t { generate, SIZE };

		namespace sck
		{
			enum class generate : uint8_t { U_K, SIZE };
		}
	}

/*!
 * \class Framer
 *
 * \brief Generates the Payload Header for DVBS2 standard.
 *
 * \tparam B: type of the bits in the Framer.
 *
 * Please use Framer for inheritance (instead of Framer).
 */
template <typename B = int>
class Framer : public Module
{
public:
	inline Task&   operator[](const frm::tsk           t) { return Module::operator[]((int)t);                          }
	inline Socket& operator[](const frm::sck::generate s) { return Module::operator[]((int)frm::tsk::generate)[(int)s]; }

protected:
	const int K; /*!< Number of information bits in one frame */

private:
	std::vector<B > PLH;
	void generate_PLH( void );
	int N_LDPC, BPS, N_XFEC_FRAME, M, N_PILOTS;

public:
	/*!
	 * \brief Constructor.
	 *
	 * \param K:        number of information bits in the frame.
	 * \param n_frames: number of frames to process in the Framer.
	 * \param name:     Framer's name.
	 */
	Framer(const int K, const int n_frames = 1);

	/*!
	 * \brief Destructor.
	 */
	virtual ~Framer() = default;

	virtual int get_K() const;

	/*!
	 * \brief Fulfills a vector with bits.
	 *
	 * \param U_K: a vector of bits to fill.
	 */
	template <class A = std::allocator<B>>
	void generate(std::vector<B,A>& U_K, std::vector<B> XFEC_frame, const int frame_id = -1);

	virtual void generate(B *U_K, B *XFEC_frame, const int frame_id = -1);

protected:
	virtual void _generate(B *U_K, B *XFEC_frame, const int frame_id);
};
}
}
#include "Framer.hxx"

#endif /* FRAMER_HPP_ */

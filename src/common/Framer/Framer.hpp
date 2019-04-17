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
	const int XFEC_FRAME_SIZE; /*!< Number of complex symbols x2 in one XFEC frame */
	const int PL_FRAME_SIZE; /*!< Number of complex symbols x2 in one payload frame */

private:

	std::vector<B > PLH; /*!< Payload header */
	void generate_PLH( void ); /*!< Payload header generation */
	int N_XFEC_FRAME, M, N_PILOTS; 
	std::string MODCOD;

public:
	/*!
	 * \brief Constructor.
	 *
	 * \param XFEC_FRAME_SIZE : Number of complex symbols in one XFEC frame.
	 * \param PL_FRAME_SIZE : Number of complex symbols in one payload frame.
	 * \param n_frames: number of frames to process in the Framer.
	 * \param name:     Framer's name.
	 */
	Framer(const int XFEC_FRAME_SIZE, const int PL_FRAME_SIZE, const std::string MODCOD, const int n_frames = 1);

	/*!
	 * \brief Destructor.
	 */
	virtual ~Framer() = default;

	//virtual int get_K() const;

	/*!
	 * \brief Generate the payload frame.
	 *
	 * \param XFEC_frame : a vector of complex input symbols.
	 * \param PL_frame : a vector of complex output symbols.
	 */
	template <class A = std::allocator<B>>
	void generate(std::vector<B,A>& XFEC_frame, std::vector<B,A>& PL_frame, const int frame_id = -1);

	virtual void generate(B *XFEC_frame, B *PL_frame, const int frame_id = -1);

protected:
	virtual void _generate(B *XFEC_frame, B *PL_frame, const int frame_id);
};
}
}
#include "Framer.hxx"

#endif /* FRAMER_HPP_ */

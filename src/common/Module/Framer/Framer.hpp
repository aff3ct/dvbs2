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
		enum class tsk : uint8_t { generate, remove_plh, SIZE };

		namespace sck
		{
			enum class generate   : uint8_t { Y_N1, Y_N2, status };
			enum class remove_plh : uint8_t { Y_N1, Y_N2, status };
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
template <typename B = float>
class Framer : public Module
{
public:
	inline Task&   operator[](const frm::tsk             t) { return Module::operator[]((int)t);                            }
	inline Socket& operator[](const frm::sck::generate   s) { return Module::operator[]((int)frm::tsk::generate  )[(int)s]; }
	inline Socket& operator[](const frm::sck::remove_plh s) { return Module::operator[]((int)frm::tsk::remove_plh)[(int)s]; }

protected:
	const int xfec_frame_size; /*!< Number of complex symbols x2 in one XFEC frame */
	const int pl_frame_size; /*!< Number of complex symbols x2 in one payload frame */

private:
	std::vector<B > plh; /*!< Payload header */
	void generate_plh( void ); /*!< Payload header generation */
	int M;
	int P;
	int N_pilots;
	std::string modcod;

public:
	/*!
	 * \brief Constructor.
	 *
	 * \param xfec_frame_size : Number of complex symbols in one XFEC frame.
	 * \param pl_frame_size : Number of complex symbols in one payload frame.
	 * \param n_frames: number of frames to process in the Framer.
	 * \param name:     Framer's name.
	 */
	Framer(const int xfec_frame_size, const int pl_frame_size, const std::string modcod, const int n_frames = 1);

	/*!
	 * \brief Destructor.
	 */
	virtual ~Framer() = default;

	virtual Framer<B>* clone() const;

	//virtual int get_K() const;

	/*!
	 * \brief Generate the payload frame.
	 *
	 * \param Y_N1 : a vector of complex input symbols.
	 * \param Y_N2 : a vector of complex output symbols.
	 */
	template <class A = std::allocator<B>>
	void generate(std::vector<B,A>& Y_N1, std::vector<B,A>& Y_N2, const int frame_id = -1);

	virtual void generate(B *Y_N1, B *Y_N2, const int frame_id = -1);

	/*!
	 * \brief Remove the payload header.
	 *
	 * \param Y_N1   : a vector of complex output symbols.
	 * \param Y_N2 : a vector of complex input symbols.
	 */

	template <class A = std::allocator<B>>
	void remove_plh(std::vector<B,A>& Y_N1, std::vector<B,A>& Y_N2, const int frame_id = -1);

	virtual void remove_plh(B *Y_N1, B *Y_N2, const int frame_id = -1);


protected:
	virtual void _generate  (B *Y_N1, B *Y_N2  , const int frame_id);
	virtual void _remove_plh(B *Y_N1  , B *Y_N2, const int frame_id);
};
}
}
#include "Framer.hxx"

#endif /* FRAMER_HPP_ */

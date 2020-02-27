/*!
 * \file
 * \brief Generates a message.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef SCRAMBLER_HPP_
#define SCRAMBLER_HPP_

#include <vector>
#include <string>
#include <iostream>

#include "Module/Module.hpp"

namespace aff3ct
{
namespace module
{
	namespace scr
	{
		enum class tsk : uint8_t { scramble, descramble, SIZE };

		namespace sck
		{
			enum class scramble   : uint8_t { X_N1, X_N2, status };
			enum class descramble : uint8_t { Y_N1, Y_N2, status };
		}
	}

/*!
 * \class Scrambler
 *
 * \brief Scrambler prototype
 *
 * \tparam D: type of the data in the Scrambler.
 */
template <typename D = int>
class Scrambler : public Module
{
public:
	inline Task&   operator[](const scr::tsk             t) { return Module::operator[]((int)t);                            }
	inline Socket& operator[](const scr::sck::scramble   s) { return Module::operator[]((int)scr::tsk::scramble  )[(int)s]; }
	inline Socket& operator[](const scr::sck::descramble s) { return Module::operator[]((int)scr::tsk::descramble)[(int)s]; }

protected:
	const int N;          // Size of one frame (= number of datas in one frame)

public:
	/*!
	 * \brief Constructor.
	 *
	 * \param N:        size of one frame.
	 * \param n_frames: number of frames to process in the Scrambler.
	 */
	Scrambler(const int N, const int n_frames = 1);

	/*!
	 * \brief Destructor.
	 */
	virtual ~Scrambler() = default;

	virtual Scrambler<D>* clone() const;

	virtual int get_N() const;

	/*!
	 * \brief Scramble the frame.
	 *
	 */
	template <class A = std::allocator<D>>
	void scramble(std::vector<D,A>& X_N1, std::vector<D,A>& X_N2, const int frame_id = -1);

	virtual void scramble(D *X_N1, D *X_N2, const int frame_id = -1);

	/*!
	 * \brief Descramble the frame.
	 *
	 */
	template <class A = std::allocator<D>>
	void descramble(std::vector<D,A>& Y_N1, std::vector<D,A>& Y_N2, const int frame_id = -1);

	virtual void descramble(D *Y_N1, D *Y_N2, const int frame_id = -1);


protected:
	virtual void _scramble  (D *X_N1, D *X_N2, const int frame_id);
	virtual void _descramble(D *Y_N1, D *Y_N2, const int frame_id);
};
}
}
#include "Scrambler.hxx"

#endif /* SCRAMBLER_HPP_ */

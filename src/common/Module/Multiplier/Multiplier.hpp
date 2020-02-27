/*!
 * \file
 * \brief Multiply signals.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef Multiplier_HPP_
#define Multiplier_HPP_

#include <string>
#include <memory>
#include <vector>

#include "Module/Module.hpp"
#include "Tools/Noise/noise_utils.h"

namespace aff3ct
{
namespace module
{
	namespace mlt
	{
		enum class tsk : uint8_t { imultiply, multiply, SIZE };

		namespace sck
		{
			enum class imultiply : uint8_t { X_N, Z_N, status };
			enum class multiply  : uint8_t { X_N, Y_N, Z_N, status };
		}
	}

/*!
 * \class Multiplier
 *
 * \brief Multiply signals.
 *
 * \tparam R: type of the reals (floating-point representation) of the input of the multiply process.
 *
 * Please use Multiplier for inheritance (instead of Multiplier)
 */
template <typename R = float>
class Multiplier : public Module
{
public:
	inline Task&   operator[](const mlt::tsk            t) { return Module::operator[]((int)t);                                }
	inline Socket& operator[](const mlt::sck::multiply  s) { return Module::operator[]((int)mlt::tsk::multiply        )[(int)s]; }
	inline Socket& operator[](const mlt::sck::imultiply s) { return Module::operator[]((int)mlt::tsk::imultiply       )[(int)s]; }

protected:
	const int N;     /*!< Size of one frame (= number of samples in one frame) */

public:
	/*!
	 * \brief Constructor.
	 *
	 * \param N:        size of one frame (= number of samples in one frame).
	 * \param n_frames: number of frames to process in the Multiplier.
	 * \param name:     Multiplier's name.
	 */
	Multiplier(const int N, const int n_frames = 1);

	void init_processes();

	/*!
	 * \brief Destructor.
	 */
	virtual ~Multiplier() = default;

	int get_N() const;

	/*!
	 * \brief Multiply a signal by a signal produced by the multiplier.
	 *
	 * By default this method does nothing.
	 *
	 * \param X_N: a vector of samples.
	 * \param Z_N: a vector of samples containing the multiplication of X_N and the internal Y_N.
	 */
	template <class AR = std::allocator<R>>
	void imultiply(const std::vector<R,AR>& X_N, std::vector<R,AR>& Z_N, const int frame_id = -1);
	virtual void imultiply(const R *X_N, R *Z_N, const int frame_id = -1);

	/*!
	 * \brief Multiply two signals together.
	 *
	 * By default this method does nothing.
	 *
	 * \param X_N: a vector of samples.
	 * \param Y_N: a vector of samples.
	 * \param Z_N: a vector of samples containing the multiplication of X_N and Y_N.
	 */
	template <class AR = std::allocator<R>>
	void multiply(const std::vector<R,AR>& X_N, const std::vector<R,AR>& Y_N, std::vector<R,AR>& Z_N, const int frame_id = -1);
	virtual void multiply(const R *X_N, const R *Y_N, R *Z_N, const int frame_id = -1);

protected:
	virtual void _imultiply(const R *X_N,  R *Z_N, const int frame_id);
	virtual void _multiply (const R *X_N,  const R *Y_N, R *Z_N, const int frame_id);
};
}
}
#include "Multiplier.hxx"

#endif /* Multiplier_HPP_ */

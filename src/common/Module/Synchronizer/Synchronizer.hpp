/*!
 * \file
 * \brief Performs synchronization on a signal.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef SYNCHONIZER_HPP_
#define SYNCHONIZER_HPP_

#include <vector>

#include "Module/Module.hpp"
#include "Tools/Noise/noise_utils.h"

namespace aff3ct
{
namespace module
{
	namespace syn
	{
		enum class tsk : uint8_t { synchronize, SIZE };

		namespace sck
		{
			enum class synchronize : uint8_t { X_N1, Y_N2, status };
		}
	}

/*!
 * \class Synchronizer
 *
 * \brief Synchronizes a signal.
 *
 * \tparam R: type of the reals (floating-point representation) of the filtering process.
 *
 * Please use Synchronizer for inheritance (instead of Synchronizer)
 */
template <typename R = float>
class Synchronizer : public Module
{
public:
	inline Task&   operator[](const syn::tsk              t) { return Module::operator[]((int)t);                             }
	inline Socket& operator[](const syn::sck::synchronize s) { return Module::operator[]((int)syn::tsk::synchronize)[(int)s]; }
protected:
	const int N_in;  /*!< Size of one frame (= number of samples in one frame) */
	const int N_out; /*!< Number of samples after the synchronization process */

public:
	/*!
	 * \brief Constructor.
	 *
	 * \param N_in:     size of one frame (= number of samples in one frame).
	 * \param N_out:    number of samples after the synchronization process.
	 * \param n_frames: number of frames to process in the Synchronizer.
	 */
	Synchronizer(const int N_in, const int N_out, const int n_frames = 1);

	void init_processes();

	/*!
	 * \brief Destructor.
	 */
	virtual ~Synchronizer() = default;

	int get_N_in() const;

	int get_N_out() const;

	virtual void reset() = 0;
	/*!
	 * \brief Synchronizes a vector of samples.
	 *
	 * By default this method does nothing.
	 *
	 * \param X_N1: a vector of samples.
	 * \param Y_N2: a synchronized vector.
	 */
	template <class AR = std::allocator<R>>
	void synchronize(const std::vector<R,AR>& X_N1, std::vector<R,AR>& Y_N2, const int frame_id = -1);

	virtual void synchronize(const R *X_N1, R *Y_N2, const int frame_id = -1);

protected:
	virtual void _synchronize(const R *X_N1,  R *Y_N2, const int frame_id);
};
}
}
#include "Synchronizer.hxx"

#endif /* SYNCHONIZER_HPP_ */

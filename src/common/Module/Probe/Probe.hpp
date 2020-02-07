/*!
 * \file
 * \brief Performs synchronization on a signal.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef PROBE_HPP_
#define PROBE_HPP_

#include <vector>

#include "Module/Module.hpp"
#include "Tools/Noise/noise_utils.h"

namespace aff3ct
{
namespace module
{
	namespace prb
	{
		enum class tsk : uint8_t { probe, SIZE };

		namespace sck
		{
			enum class probe : uint8_t { X_N, SIZE };
		}
	}

/*!
 * \class Probe
 *
 * \brief Synchronizes a signal.
 *
 * \tparam R: type of the reals (floating-point representation) of the filtering process.
 *
 * Please use Probe for inheritance (instead of Probe)
 */
template <typename R = float>
class Probe : public Module
{
public:
	inline Task&   operator[](const prb::tsk        t) { return Module::operator[]((int)t);                      }
	inline Socket& operator[](const prb::sck::probe s) { return Module::operator[]((int)prb::tsk::probe)[(int)s];}


protected:
	const int N;  /*!< Size of one frame (= number of samples in one frame) */

public:
	/*!
	 * \brief Constructor.
	 *
	 * \param N:        size of one frame (= number of samples in one frame).
	 * \param n_frames: number of frames to process in the Probe.
	 */
	Probe(const int N, const int n_frames = 1);

	void init_processes();

	/*!
	 * \brief Destructor.
	 */
	virtual ~Probe() = default;

	virtual void reset() = 0;
	/*!
	 * \brief Probes a of Module.
	 *
	 * By default this method does nothing.
	 *
	 * \param X_N: a vector of samples.
	 */
	template <class AR = std::allocator<R>>
	void probe(std::vector<R,AR>& X_N, const int frame_id = -1);

	virtual void probe(R *X_N, const int frame_id = -1);

protected:
	virtual void _probe(R *X_N, const int frame_id);
};
}
}
#include "Probe.hxx"

#endif /* PROBE_HPP_ */

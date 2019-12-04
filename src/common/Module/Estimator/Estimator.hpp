/*!
 * \file
 * \brief Generates a message.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef ESTIMATOR_HPP
#define ESTIMATOR_HPP

#include <vector>
#include <string>
#include <iostream>

#include "Module/Module.hpp"

namespace aff3ct
{
namespace module
{
	namespace est
	{
		enum class tsk : uint8_t { estimate, SIZE };

		namespace sck
		{
			enum class estimate   : uint8_t { X_N, H_N, SIZE };
		}
	}

/*!
 * \class Estimator
 *
 * \brief Estimator prototype
 *
 * \tparam D: type of the data in the Estimator.
 */
template <typename R = float>
class Estimator : public Module
{
public:
	inline Task&   operator[](const est::tsk             t) { return Module::operator[]((int)t);                            }
	inline Socket& operator[](const est::sck::estimate   s) { return Module::operator[]((int)est::tsk::estimate)[(int)s]; }

protected:
	const int N;                 // Size of one frame (= number of datas in one frame)
	const int bps;               // bits per symbol for eb_n0 computation
	const float code_rate;       // Code rate for eb_n0 computation
	const tools::Noise<> *noise; // the estimated noise
	float sigma_n2;

public:
	/*!
	 * \brief Constructor.
	 *
	 * \param N:        size of one frame.
	 * \param n_frames: number of frames to process in the Estimator.
	 */
	Estimator(const int N, const float code_rate, const int bps, const int n_frames = 1);

	/*!
	 * \brief Destructor.
	 */
	virtual ~Estimator() = default;

	virtual int get_N() const;

	R get_sigma_n2();

	const tools::Noise<>& get_noise() const;

	virtual void set_noise(const tools::Noise<>& noise);

	/*!
	 * \brief Estimate the noise the frame.
	 *
	 */
	template <class A = std::allocator<R>>
	void estimate(std::vector<R,A>& X_N, std::vector<R,A>& H_N, const int frame_id = -1);

	virtual void estimate(R *X_N, R *H_N, const int frame_id = -1);

protected:
	virtual void _estimate  (R *X_N, R *H_N, const int frame_id);
};
}
}
#include "Estimator.hxx"

#endif /* ESTIMATOR_HPP */

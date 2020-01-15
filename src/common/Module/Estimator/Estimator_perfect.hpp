/*!
 * \file
 * \brief Generates a message.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef ESTIMATOR_PERFECT_HPP
#define ESTIMATOR_PERFECT_HPP

#include "Module/Estimator/Estimator.hpp"

namespace aff3ct
{
namespace module
{
/*!
 * \class Estimator_perfect
 *
 * \brief Perfect estimator
 *
 * \tparam R: type of the data in the Estimator.
 */
template <typename R = float>
class Estimator_perfect : public Estimator<R>,
                          public tools::Interface_notify_noise_update
{
protected:
	tools::Noise<R>* noise_ref; // bits per symbol for eb_n0 computation

public:
	/*!
	 * \brief Constructor.
	 *
	 * \param N:         size of one frame.
	 * \param noise_ref: the noise from which values will be copied
	 * \param n_frames:  number of frames to process in the Estimator.
	 */
	Estimator_perfect(const int N, tools::Noise<R>* noise_ref, const int n_frames = 1);

	/*!
	 * \brief Destructor.
	 */
	virtual ~Estimator_perfect() = default;

	virtual Estimator_perfect<R>* clone() const;

	virtual void set_noise_ref(tools::Noise<>& noise);

	tools::Noise<>& get_noise_ref() const;

	virtual void notify_noise_update();

	void check_noise(); // check that the noise has the expected type
	void check_noise_ref(); // check that the reference noise has the expected type

protected:
	virtual void _estimate  (const R *X_N, R *H_N, const int frame_id);
};
}
}
#include "Estimator_perfect.hxx"

#endif /* ESTIMATOR_PERFECT_HPP */

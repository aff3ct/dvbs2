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
class Estimator_perfect : public Estimator<R>
{
protected:
	tools::Sigma<R>* noise_ref; // bits per symbol for eb_n0 computation

public:
	/*!
	 * \brief Constructor.
	 *
	 * \param N:         size of one frame.
	 * \param noise_ref: the noise from which values will be copied
	 * \param n_frames:  number of frames to process in the Estimator.
	 */
	Estimator_perfect(const int N, tools::Sigma<R>* noise_ref, const int n_frames = 1);

	/*!
	 * \brief Destructor.
	 */
	virtual ~Estimator_perfect() = default;

	virtual Estimator_perfect<R>* clone() const;

	virtual void set_noise_ref(tools::Sigma<>& noise);

	tools::Sigma<>& get_noise_ref() const;

protected:
	virtual void _estimate(const R *X_N,         const int frame_id);
	virtual void _rescale (const R *X_N, R *Y_N, const int frame_id);
};
}
}
#include "Estimator_perfect.hxx"

#endif /* ESTIMATOR_PERFECT_HPP */

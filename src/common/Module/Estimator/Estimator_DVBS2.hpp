#ifndef ESTIMATOR_DVBS2_HPP
#define ESTIMATOR_DVBS2_HPP

#include "Module/Estimator/Estimator.hpp"

namespace aff3ct
{
namespace module
{
/*!
 * \class Estimator_DVBS2
 *
 * \brief Estimator with moments method for sigma estimation.
 *
 * \tparam R: type of the data in the Estimator.
 */
template <typename R = float>
class Estimator_DVBS2 : public Estimator<R>
{
protected:
	const int bps;         // bits per symbol for eb_n0 computation
	const float code_rate; // Code rate for eb_n0 computation

public:
	/*!
	 * \brief Constructor.
	 *
	 * \param N:         size of one frame.
	 * \param code_rate: code rate.
	 * \param bps:       number of bits per symbol.
	 * \param n_frames:  number of frames to process in the Estimator.
	 */
	Estimator_DVBS2(const int N, const float code_rate, const int bps, const int n_frames = 1);

	/*!
	 * \brief Destructor.
	 */
	virtual ~Estimator_DVBS2() = default;

	virtual Estimator_DVBS2<R>* clone() const;

protected:
	virtual void _estimate(const R *X_N, const int frame_id);
};
}
}
#include "Estimator_DVBS2.hxx"

#endif /* ESTIMATOR_DVBS2_HPP */

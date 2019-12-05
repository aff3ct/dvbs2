/*!
 * \file
 * \brief Generates a message.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef ESTIMATOR_DVBS2O_HPP
#define ESTIMATOR_DVBS2O_HPP

#include <vector>
#include <string>
#include <iostream>

#include "Module/Module.hpp"

namespace aff3ct
{
namespace module
{
/*!
 * \class Estimator_DVBS2O
 *
 * \brief Estimator with moments method for sigma estimation.
 *
 * \tparam R: type of the data in the Estimator.
 */
template <typename R = float>
class Estimator_DVBS2O : public Estimator<R>
{
protected:
	const int bps;               // bits per symbol for eb_n0 computation
	const float code_rate;       // Code rate for eb_n0 computation

public:
	/*!
	 * \brief Constructor.
	 *
	 * \param N:         size of one frame.
	 * \param code_rate: code rate.
	 * \param bps:       number of bits per symbol.
	 * \param n_frames:  number of frames to process in the Estimator.
	 */
	Estimator_DVBS2O(const int N, const float code_rate, const int bps, const int n_frames = 1);

	/*!
	 * \brief Destructor.
	 */
	virtual ~Estimator_DVBS2O() = default;

	void check_noise(); // check that the noise has the expected type

protected:
	virtual void _estimate  (R *X_N, R *H_N, const int frame_id);
};
}
}
#include "Estimator_DVBS2O.hxx"

#endif /* ESTIMATOR_DVBS2O_HPP */

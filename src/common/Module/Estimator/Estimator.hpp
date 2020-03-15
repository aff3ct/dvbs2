#ifndef ESTIMATOR_HPP
#define ESTIMATOR_HPP

#include <vector>
#include <string>
#include <iostream>

#include "Module/Estimator/Estimator.hpp"
#include "Tools/Noise/Noise.hpp"

namespace aff3ct
{
namespace module
{
	namespace est
	{
		enum class tsk : uint8_t { estimate, rescale, SIZE };

		namespace sck
		{
			enum class estimate : uint8_t { X_N, SIG, Eb_N0, Es_N0, H_N, status };
			enum class rescale  : uint8_t { X_N, SIG, Eb_N0, Es_N0, H_N, Y_N, status };
		}
	}

/*!
 * \class Estimator
 *
 * \brief Estimator prototype
 *
 * \tparam R: type of the data in the Estimator.
 */
template <typename R = float>
class Estimator : public Module
{
public:
	inline Task&   operator[](const est::tsk             t) { return Module::operator[]((int)t);                            }
	inline Socket& operator[](const est::sck::estimate   s) { return Module::operator[]((int)est::tsk::estimate)[(int)s]; }

	inline Socket& operator[](const est::sck::rescale    s) { return Module::operator[]((int)est::tsk::rescale)[(int)s]; }


protected:
	const int N;           // Size of one frame (= number of datas in one frame)
	tools::Noise<> *noise; // the estimated noise
	R sigma_estimated;
	R ebn0_estimated;
	R esn0_estimated;
public:
	/*!
	 * \brief Constructor.
	 *
	 * \param N:        size of one frame.
	 * \param n_frames: number of frames to process in the Estimator.
	 */
	Estimator(const int N, const int n_frames = 1);

	/*!
	 * \brief Destructor.
	 */
	virtual ~Estimator() = default;

	virtual Estimator<R>* clone() const;

	virtual int get_N() const;

	tools::Noise<>& get_noise() const;

	virtual void set_noise(tools::Noise<>& noise);

	void check_noise(); // check that the noise has the expected type

	/*!
	 * \brief Estimate the noise the frame.
	 *
	 */
	template <class A = std::allocator<R>>
	void estimate(const std::vector<R,A>& X_N,
	                    std::vector<R,A>& SIG,
	                    std::vector<R,A>& Eb_N0,
	                    std::vector<R,A>& Es_N0,
	                    std::vector<R,A>& H_N,
	              const int frame_id = -1);

	virtual void estimate(const R *X_N, R *SIG, R *Eb_N0, R *Es_N0, R *H_N, const int frame_id = -1);


	/*!
	 * \brief Estimate the noise the frame and rescale the signal.
	 *
	 */
	template <class A = std::allocator<R>>
	void rescale(const std::vector<R,A>& X_N,
	                   std::vector<R,A>& SIG,
	                   std::vector<R,A>& Eb_N0,
	                   std::vector<R,A>& Es_N0,
	                   std::vector<R,A>& H_N,
	                   std::vector<R,A>& Y_N,
	             const int frame_id = -1);

	virtual void rescale(const R *X_N, R *SIG, R *Eb_N0, R *Es_N0, R *H_N, R *Y_N, const int frame_id = -1);

	void reset();

protected:
	virtual void _estimate(const R *X_N, R *H_N,         const int frame_id);
	virtual void _rescale (const R *X_N, R *H_N, R *Y_N, const int frame_id);
};
}
}
#include "Estimator.hxx"

#endif /* ESTIMATOR_HPP */

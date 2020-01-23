#ifndef SYNCHRONIZER_FRAME_HPP
#define SYNCHRONIZER_FRAME_HPP

#include <vector>
#include <complex>

#include "Module/Synchronizer/Synchronizer.hpp"

namespace aff3ct
{
namespace module
{

namespace sfm
{
	enum class tsk : uint8_t { synchronize, SIZE };

	namespace sck
	{
		enum class synchronize : uint8_t { X_N1, delay, Y_N2, SIZE };
	}
}
template <typename R = float>
class Synchronizer_frame : public Module
{
protected:
	const int N_in;  /*!< Size of one frame (= number of samples in one frame) */
	const int N_out; /*!< Number of samples after the synchronization process */

public:
	inline Task&   operator[](const sfm::tsk              t) { return Module::operator[]((int)t);                             }
	inline Socket& operator[](const sfm::sck::synchronize s) { return Module::operator[]((int)syn::tsk::synchronize)[(int)s]; }

public:
	Synchronizer_frame(const int N, const int n_frames = 1);
	virtual ~Synchronizer_frame() = default;

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
	void synchronize(const std::vector<R,AR>& X_N1, std::vector<int>& delay, std::vector<R,AR>& Y_N2, const int frame_id = -1);

	virtual void synchronize(const R *X_N1, int* delay, R *Y_N2, const int frame_id = -1);

protected:
	virtual void _synchronize(const R *X_N1, int* delay, R *Y_N2, const int frame_id) = 0;
};

}
}
#include "Synchronizer_frame.hxx"
#endif //SYNCHRONIZER_FRAME_HPP

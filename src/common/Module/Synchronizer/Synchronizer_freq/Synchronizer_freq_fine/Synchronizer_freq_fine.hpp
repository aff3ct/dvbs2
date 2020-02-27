#ifndef SYNCHRONIZER_FREQ_FINE_HPP
#define SYNCHRONIZER_FREQ_FINE_HPP

#include <vector>
#include <complex>

#include "Module/Module.hpp"

namespace aff3ct
{
namespace module
{
	namespace sff
	{
		enum class tsk : uint8_t { synchronize, SIZE };

		namespace sck
		{
			enum class synchronize : uint8_t { X_N1, FRQ, PHS, Y_N2, status };
		}
	}

template <typename R = float>
class Synchronizer_freq_fine : public Module
{
public:
	inline Task&   operator[](const sff::tsk                      t) { return Module::operator[]((int)t);                                     }
	inline Socket& operator[](const sff::sck::synchronize         s) { return Module::operator[]((int)sff::tsk::synchronize        )[(int)s]; }

protected:
	const int N;  /*!< Size of one frame (= number of samples in one frame) */

public:
	Synchronizer_freq_fine(const int N, const int n_frames = 1);
	virtual ~Synchronizer_freq_fine() = default;

	virtual Synchronizer_freq_fine<R>* clone() const;

	R get_estimated_freq () const {return this->estimated_freq; };
	R get_estimated_phase() const {return this->estimated_phase;};

	int get_N() const;

	void reset();
	/*!
	 * \brief Synchronizes a vector of samples.
	 *
	 * By default this method does nothing.
	 *
	 * \param X_N1: a vector of samples.
	 * \param Y_N2: a synchronized vector.
	 */
	template <class AR = std::allocator<R>>
	void synchronize(const std::vector<R,AR>& X_N1, std::vector<R,AR>& FRQ, std::vector<R,AR>& PHS, std::vector<R,AR>& Y_N2, const int frame_id = -1);

	virtual void synchronize(const R *X_N1, R * FRQ, R* PHS, R *Y_N2, const int frame_id = -1);

protected:
	R estimated_freq;
	R estimated_phase;

	virtual void _synchronize(const R *X_N1,  R *Y_N2, const int frame_id) = 0;
	virtual void _reset      (                                           ) = 0;
};

}
}
#include "Synchronizer_freq_fine.hxx"
#endif //SYNCHRONIZER_FREQ_FINE_HPP

#ifndef SYNCHRONIZER_FREQ_COARSE_HPP
#define SYNCHRONIZER_FREQ_COARSE_HPP

#include <vector>
#include <complex>

#include "Module/Module.hpp"

namespace aff3ct
{
namespace module
{
	namespace sfc
	{
		enum class tsk : uint8_t { synchronize, SIZE };

		namespace sck
		{
			enum class synchronize : uint8_t { X_N1, FRQ, PHS, Y_N2, status };
		}
	}

template <typename R = float>
class Synchronizer_freq_coarse : public Module
{
public:
	inline Task&   operator[](const sfc::tsk                      t) { return Module::operator[]((int)t);                                     }
	inline Socket& operator[](const sfc::sck::synchronize         s) { return Module::operator[]((int)sfc::tsk::synchronize        )[(int)s]; }

protected:
	const int N;  /*!< Size of one frame (= number of samples in one frame) */
	bool is_active;
	int curr_idx;
	R estimated_freq;
	R estimated_phase;

public:
	Synchronizer_freq_coarse(const int N, const int n_frames = 1);
	virtual ~Synchronizer_freq_coarse() = default;

	virtual void update_phase(const std::complex<R> spl) = 0;
	virtual void step (const std::complex<R>* x_elt, std::complex<R>* y_elt) = 0;
	virtual void set_PLL_coeffs (const int samples_per_symbol, const R damping_factor, const R normalized_bandwidth) = 0;

	void set_curr_idx(int curr_idx) {this->curr_idx = curr_idx;};
	int get_curr_idx() {return this->curr_idx;};
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

	virtual void synchronize(const R *X_N1, R *FRQ, R* PHS, R *Y_N2, const int frame_id = -1);

protected:
	virtual void _synchronize(const R *X_N1,  R *Y_N2, const int frame_id) = 0;
	virtual void _reset      (                                           ) = 0;
};

}
}
#include "Synchronizer_freq_coarse.hxx"
#endif //SYNCHRONIZER_FREQ_COARSE_HPP

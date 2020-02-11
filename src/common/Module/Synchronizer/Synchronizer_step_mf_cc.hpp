#ifndef SYNCHRONIZER_STEP_MF_CC_HPP
#define SYNCHRONIZER_STEP_MF_CC_HPP

#include <vector>
#include <complex>
#include <aff3ct.hpp>

#include "Module/Synchronizer/Synchronizer.hpp"
#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_timing.hpp"
#include "Module/Synchronizer/Synchronizer_freq/Synchronizer_freq_coarse/Synchronizer_freq_coarse.hpp"
#include "Module/Filter/Filter_FIR/Filter_RRC/Filter_RRC_ccr_naive.hpp"

namespace aff3ct
{
namespace module
{
	namespace smf
	{
		enum class tsk : uint8_t { synchronize, SIZE };

		namespace sck
		{
			enum class synchronize : uint8_t { delay, X_N1, Y_N1, B_N1, status };
		}
	}

/*!
 * \class Synchronizer
 *
 * \brief Synchronizes a signal.
 *
 * \tparam R: type of the reals (floating-point representation) of the filtering process.
 *
 * Please use Synchronizer for inheritance (instead of Synchronizer)
 */
template <typename B = int, typename R = float>
class Synchronizer_step_mf_cc : public Module
{
public:
	inline Task&   operator[](const smf::tsk              t) {return Module::operator[]((int)t);                            }
	inline Socket& operator[](const smf::sck::synchronize s) {return Module::operator[]((int)smf::tsk::synchronize)[(int)s];}

private:
	int last_delay;

protected:
	const int N_in;  /*!< Size of one frame (= number of samples in one frame) */
	const int N_out; /*!< Number of samples after the synchronization process */

	std::vector<R> Y_N1_tmp;
	std::vector<B> B_N1_tmp;

public:
	Synchronizer_step_mf_cc (aff3ct::module::Synchronizer_freq_coarse<R> *sync_coarse_f,
	                         aff3ct::module::Filter_RRC_ccr_naive<R>     *matched_filter,
	                         aff3ct::module::Synchronizer_timing<B,R>    *sync_timing,
	                         const int n_frames = 1);

	virtual ~Synchronizer_step_mf_cc();

	aff3ct::module::Synchronizer_freq_coarse<R> *sync_coarse_f;
	aff3ct::module::Filter_RRC_ccr_naive<R>     *matched_filter;
	aff3ct::module::Synchronizer_timing<B,R>    *sync_timing;

	int get_N_in() const;

	int get_N_out() const;

	int get_delay();

	void reset();
	/*!
	 * \brief Synchronizes a vector of samples.
	 *
	 * By default this method does nothing.
	 *
	 * \param X_N1: a vector of samples.
	 * \param Y_N2: a synchronized vector.
	 */
	template <class AB = std::allocator<B>, class AR = std::allocator<R>>
	void synchronize(const std::vector<int>& delay, const std::vector<R,AR>& X_N1, std::vector<R,AR>& Y_N1, std::vector<B,AB>& B_N1, const int frame_id = -1);

	virtual void synchronize(const int* delay, const R *X_N1, R *Y_N1, B *B_N1, const int frame_id = -1);

protected:
	virtual void _synchronize(const int* delay, const R *X_N1, R *Y_N1, B *B_N1, const int frame_id);
};

}
}

#endif //SYNCHRONIZER_STEP_MF_CC_HPP

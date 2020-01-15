#ifndef SYNCHRONIZER_TIMING
#define SYNCHRONIZER_TIMING

#include <complex>

#include "Module/Synchronizer/Synchronizer.hpp"

namespace aff3ct
{
namespace module
{
	namespace stm
	{
		enum class tsk : uint8_t {synchronize, SIZE };

		namespace sck
		{
			enum class synchronize : uint8_t { X_N1, Y_N2, SIZE };
		}
	}

/*!
 * \class Synchronizer
 *
 * \brief Synchronizes a signal in the time domain.
 *
 * \tparam R: type of the reals (floating-point representation) of the filtering process.
 *
 * Please use Synchronizer for inheritance (instead of Synchronizer)
 */
template <typename R = float>
class Synchronizer_timing : public Module
{
public:
	inline Task&   operator[](const stm::tsk              t) { return Module::operator[]((int)t);                             }
	inline Socket& operator[](const stm::sck::synchronize s) { return Module::operator[]((int)stm::tsk::synchronize)[(int)s]; }

	Synchronizer_timing (const int N, const int osf, const int n_frames = 1);
	virtual ~Synchronizer_timing() = default;

	void reset();

	virtual void step (const std::complex<R> *X_N1) = 0;

	R               get_mu            ();
	std::complex<R> get_last_symbol   ();
	int             get_is_strobe     ();
	int             get_overflow_cnt  ();
	int             get_underflow_cnt ();
	int             get_delay         ();
	bool            can_pull          ();

	int get_N_in() const;

	int get_N_out() const;

	/*!
	 * \brief Synchronizes a vector of samples.
	 *
	 * By default this method does nothing.
	 *
	 * \param X_N1: a vector of samples.
	 * \param Y_N2: a synchronized vector.
	 */
	template <class AR = std::allocator<R>>
	void synchronize(const std::vector<R,AR>& X_N1, std::vector<R,AR>& Y_N2, const int frame_id = -1);

	virtual void synchronize(const R *X_N1, R *Y_N2, const int frame_id = -1);

	void pull(std::complex<R> *strobe);

protected:

	const int N_in;  /*!< Size of one frame (= number of samples in one frame) */
	const int N_out; /*!< Number of samples after the synchronization process */

	const int osf;
	const int POW_osf;
	const R   INV_osf;

	std::complex<R> last_symbol;
	R mu;
	int is_strobe;

	int overflow_cnt;
	int underflow_cnt;

	std::vector<std::complex<R> > output_buffer;
	int outbuf_head;
	int outbuf_tail;
	int outbuf_max_sz;
	int outbuf_cur_sz;

	virtual void _synchronize(const R *X_N1, R *Y_N2, const int frame_id);
	virtual void _reset      (                                          ) = 0;

	void push(const std::complex<R> strobe);
};

}
}

#include "Synchronizer_timing.hxx"

#endif //SYNCHRONIZER_TIMING

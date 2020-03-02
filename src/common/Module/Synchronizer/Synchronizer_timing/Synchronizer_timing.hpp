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
		enum class tsk : uint8_t {synchronize, extract, SIZE };

		namespace sck
		{
			enum class synchronize : uint8_t { X_N1, MU, Y_N1, B_N1, status };
			enum class extract     : uint8_t { Y_N1, B_N1, UFW, Y_N2, status };
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
template <typename B = int, typename R = float>
class Synchronizer_timing : public Module
{
public:
	inline Task&   operator[](const stm::tsk              t) { return Module::operator[]((int)t);                             }
	inline Socket& operator[](const stm::sck::synchronize s) { return Module::operator[]((int)stm::tsk::synchronize)[(int)s]; }
	inline Socket& operator[](const stm::sck::extract     s) { return Module::operator[]((int)stm::tsk::extract    )[(int)s]; }

protected:
	const int N_in;  /*!< Size of one frame (= number of samples in one frame) */
	const int N_out; /*!< Number of samples after the synchronization process */

	const int osf;
	const int POW_osf;
	const R   INV_osf;

	std::complex<R> last_symbol;
	R mu;
	int is_strobe;

	std::vector<B> underflow_cnt;

	std::vector<R> output_buffer;
	int outbuf_head;
	int outbuf_max_sz;
	int outbuf_cur_sz;

	bool act;

public:
	Synchronizer_timing (const int N, const int osf, const int n_frames = 1);
	virtual ~Synchronizer_timing() = default;

	void reset();

	virtual void step(const std::complex<R> *X_N1, std::complex<R>* Y_N1, B* B_N1) = 0;

	R               get_mu           () const;
	std::complex<R> get_last_symbol  ();
	int             get_is_strobe    ();
	int             get_delay        ();
	int             get_N_in         () const;
	int             get_N_out        () const;
	bool            can_pull         ();
	void            set_act          (bool act) {this->act = act;};
	/*!
	 * \brief Pushes a vector of samples in the inner buffer
	 *
	 * By default this method does nothing.
	 *
	 * \param X_N1: a vector of samples.
	 */
	template <class AB = std::allocator<B>, class AR = std::allocator<R>>
	void extract(const std::vector<R,AR>& Y_N1, const std::vector<B,AB>& B_N1, std::vector<B,AB>& UFW, std::vector<R,AR>& Y_N2, const int frame_id = -1);

	void extract(const R *Y_N1, const B *B_N1, B *UFW, R *Y_N2, const int frame_id = -1);

	/*!
	 * \brief Synchronizes a vector of samples.
	 *
	 * By default this method does nothing.
	 *
	 * \param X_N1: a vector of samples.
	 */
	template <class AB = std::allocator<B>, class AR = std::allocator<R>>
	void synchronize(const std::vector<R,AR>& X_N1,
	                 std::vector<R,AR>& MU,
	                 std::vector<R,AR>& Y_N1,
	                 std::vector<B,AB>& B_N1,
	                 const int frame_id = -1);

	void synchronize(const R *X_N1, R *MU, R *Y_N1, B* B_N1, const int frame_id = -1);

protected:
	virtual void _reset      () = 0;
	virtual void _synchronize(const R *X_N1, R* Y_N1, B * B_N1, const int frame_id) = 0;
	virtual void _extract    (const R* Y_N1, const B * B_N1, R* Y_N2, const int start_frame_id, const int n_frames);
};

}
}

#include "Synchronizer_timing.hxx"

#endif //SYNCHRONIZER_TIMING

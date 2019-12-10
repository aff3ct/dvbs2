#ifndef SYNCHRONIZER_TIMING
#define SYNCHRONIZER_TIMING

#include <complex>

#include "Module/Synchronizer/Synchronizer.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Synchronizer_timing : public Synchronizer<R>
{
public:
	Synchronizer_timing (const int N, const int osf, const int n_frames = 1);
	virtual ~Synchronizer_timing() = default;

	void reset();

	virtual void step (const std::complex<R> *X_N1) = 0;

	R               get_mu            (){return this->mu;           };
	std::complex<R> get_last_symbol   (){return this->last_symbol;  };
	int             get_is_strobe     (){return this->is_strobe;    };
	int             get_overflow_cnt  (){return this->overflow_cnt; };
	int             get_underflow_cnt (){return this->underflow_cnt;};
	int             get_delay         (){return this->outbuf_cur_sz;};
	bool            can_pull          (){return this->outbuf_cur_sz > this->N_out/2;};

	void pull(std::complex<R> *strobe);

protected:
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


	virtual void _synchronize(const R *X_N1,  R *Y_N2, const int frame_id) = 0;
	virtual void _reset      (                                           ) = 0;

	void push(const std::complex<R> strobe);

};

}
}

#include "Synchronizer_timing.hxx"

#endif //SYNCHRONIZER_TIMING

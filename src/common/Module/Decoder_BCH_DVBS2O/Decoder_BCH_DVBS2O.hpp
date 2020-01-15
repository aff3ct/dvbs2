#ifndef DECODER_BCH_DVBS2O_HPP_
#define DECODER_BCH_DVBS2O_HPP_

#include <aff3ct.hpp>

namespace aff3ct
{
namespace module
{
template <typename B = int, typename R = float>
class Decoder_BCH_DVBS2O : public Decoder_BCH_std<B, R>
{
public:
	Decoder_BCH_DVBS2O(const int& K, const int& N, const tools::BCH_polynomial_generator<B>& GF, const int n_frames = 1);

	virtual ~Decoder_BCH_DVBS2O() = default;

	virtual Decoder_BCH_DVBS2O<B,R>* clone() const;

protected:
	virtual void _decode_hiho   (const B *Y_N, B *V_K, const int frame_id);
	virtual void _decode_hiho_cw(const B *Y_N, B *V_N, const int frame_id);
	virtual void _decode_siho   (const R *Y_N, B *V_K, const int frame_id);
	virtual void _decode_siho_cw(const R *Y_N, B *V_N, const int frame_id);
};
}
}

#endif // DECODER_BCH_DVBS2O_HPP_

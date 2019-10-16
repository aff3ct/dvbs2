#ifndef ENCODER_BCH_DVBS2O_HPP_
#define ENCODER_BCH_DVBS2O_HPP_

#include <aff3ct.hpp>

namespace aff3ct
{
namespace module
{
template <typename B = int>
class Encoder_BCH_DVBS2O : public Encoder_BCH<B>
{
protected:
	std::vector<B> U_K_rev;
public:
	Encoder_BCH_DVBS2O(const int& K, const int& N, const tools::BCH_polynomial_generator<B>& GF, const int n_frames = 1);

	virtual ~Encoder_BCH_DVBS2O() = default;

protected:
	virtual void  _encode(const B *U_K, B *X_N, const int frame_id);
};
}
}

#endif // ENCODER_BCH_DVBS2O_HPP_

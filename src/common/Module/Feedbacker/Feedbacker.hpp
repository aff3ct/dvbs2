#ifndef FEEDBACKER_HPP_
#define FEEDBACKER_HPP_

#include <vector>
#include <string>
#include <iostream>

#include "Module/Module.hpp"

namespace aff3ct
{
namespace module
{
	namespace fbr
	{
		enum class tsk : uint8_t { memorize, produce, SIZE };

		namespace sck
		{
			enum class memorize : uint8_t { X_N, status };
			enum class produce : uint8_t { Y_N, status };
		}
	}

template <typename D = int>
class Feedbacker : public Module
{
public:
	inline Task&   operator[](const fbr::tsk           t) { return Module::operator[]((int)t);                          }
	inline Socket& operator[](const fbr::sck::memorize s) { return Module::operator[]((int)fbr::tsk::memorize)[(int)s]; }
	inline Socket& operator[](const fbr::sck::produce  s) { return Module::operator[]((int)fbr::tsk::produce )[(int)s]; }

protected:
	const int N; // Size of one frame (= number of datas in one frame)
	const D init_val;
	std::vector<D> data;

public:
	Feedbacker(const int N, const D init_val);

	/*!
	 * \brief Destructor.
	 */
	virtual ~Feedbacker() = default;

	virtual Feedbacker<D>* clone() const;

	virtual int get_N() const;

	virtual void set_n_frames(const size_t n_frames);

protected:
	virtual void _memorize(const D *X_N, const size_t frame_id);
	virtual void _produce (      D *Y_N, const size_t frame_id);
};
}
}
#include "Feedbacker.hxx"

#endif /* FEEDBACKER_HPP_ */

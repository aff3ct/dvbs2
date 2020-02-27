#ifndef PROBE_LATENCY_HPP_
#define PROBE_LATENCY_HPP_

#include <string>
#include <vector>
#include <chrono>
#include <typeindex>

#include "Module/Probe/Probe.hpp"
#include "Tools/Reporter/Reporter_probe.hpp"

namespace aff3ct
{
namespace module
{
template <typename T>
class Probe_latency : public Probe<T>
{
protected:
	std::chrono::time_point<std::chrono::steady_clock> t_start;

public:
	Probe_latency(const int size, const std::string &col_name, tools::Reporter_probe& reporter, const int n_frames = 1);

	virtual ~Probe_latency() = default;

	virtual std::type_index get_datatype() const;

	virtual void probe(const T *in, const int frame_id);

	virtual void reset();
};
}
}

#endif /* PROBE_LATENCY_HPP_ */

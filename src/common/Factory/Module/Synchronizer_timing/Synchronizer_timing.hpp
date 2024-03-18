#ifndef FACTORY_SYNCHRONIZER_TIMING_HPP_
#define FACTORY_SYNCHRONIZER_TIMING_HPP_

#include <string>
#include <memory>
#include <aff3ct.hpp>

#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_timing.hpp"

namespace aff3ct
{
namespace factory
{
extern const std::string Synchronizer_timing_name;
extern const std::string Synchronizer_timing_prefix;
struct Synchronizer_timing : Factory
{
public:
	// ------------------------------------------------------------------------------------------------- PARAMETERS
	// required parameters

	// optional parameters
	int   N                    = 0;
	int   n_frames             = 1;
	int   osf                  = 2;
	int   hold_size            = 101;
	float ref_delay            = 0;
	float damping_factor       = std::sqrt(0.5);
	float normalized_bandwidth = (float)5e-5;
	float detector_gain        = (float)2;
	std::string type           = "FAST";

	// deduced parameters

	// ---------------------------------------------------------------------------------------------------- METHODS
	explicit Synchronizer_timing(const std::string &p = Synchronizer_timing_prefix);
	virtual ~Synchronizer_timing() = default;
	Synchronizer_timing* clone() const;

	// parameters construction
	virtual void get_description(cli::Argument_map_info &args) const;
	virtual void store          (const cli::Argument_map_value &vals);
	virtual void get_headers    (std::map<std::string,tools::header_list>& headers, const bool full = true) const;


	template <typename B=int, typename R=float>
	module::Synchronizer_timing<B,R>* build() const;

};
}
}

#endif /* FACTORY_SYNCHRONIZER_TIMING_HPP_ */


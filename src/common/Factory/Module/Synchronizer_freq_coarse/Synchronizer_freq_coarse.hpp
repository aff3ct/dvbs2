#ifndef FACTORY_SYNCHRONIZER_FREQUENCY_COARSE_HPP_
#define FACTORY_SYNCHRONIZER_FREQUENCY_COARSE_HPP_

#include <string>
#include <memory>
#include <aff3ct.hpp>

#include "Module/Synchronizer/Synchronizer_freq/Synchronizer_freq_coarse/Synchronizer_freq_coarse.hpp"

namespace aff3ct
{
namespace factory
{
extern const std::string Synchronizer_freq_coarse_name;
extern const std::string Synchronizer_freq_coarse_prefix;
struct Synchronizer_freq_coarse : Factory
{
public:
	// ------------------------------------------------------------------------------------------------- PARAMETERS
	// required parameters

	// optional parameters
	int   N          = 0;
	int   n_frames   = 1;
	int   osf        = 2;
	float damping_factor       = std::sqrt(0.5);
	float normalized_bandwidth = (float)1e-4;
	float ref_freq_shift       = (float)0;
	std::string type = "NORMAL";

	// deduced parameters

	// ---------------------------------------------------------------------------------------------------- METHODS
	explicit Synchronizer_freq_coarse(const std::string &p = Synchronizer_freq_coarse_prefix);
	virtual ~Synchronizer_freq_coarse() = default;
	Synchronizer_freq_coarse* clone() const;

	// parameters construction
	virtual void get_description(cli::Argument_map_info &args) const;
	virtual void store          (const cli::Argument_map_value &vals);
	virtual void get_headers    (std::map<std::string,header_list>& headers, const bool full = true) const;


	template <typename R = float>
	module::Synchronizer_freq_coarse<R>* build() const;

};
}
}

#endif /* FACTORY_SYNCHRONIZER_FREQUENCY_COARSE_HPP_ */


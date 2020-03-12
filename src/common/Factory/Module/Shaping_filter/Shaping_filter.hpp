#ifndef FACTORY_SHAPING_FILTER_HPP_
#define FACTORY_SHAPING_FILTER_HPP_

#include <string>
#include <memory>
#include <aff3ct.hpp>

#include "Module/Filter/Filter_UPFIR/Filter_UPRRC/Filter_UPRRC_ccr_naive.hpp"
#include "Module/Filter/Filter_FIR/Filter_RRC/Filter_RRC_ccr_naive.hpp"

namespace aff3ct
{
namespace factory
{
extern const std::string Shaping_filter_name;
extern const std::string Shaping_filter_prefix;
struct Shaping_filter : Factory
{
public:
	// ------------------------------------------------------------------------------------------------- PARAMETERS
	// required parameters

	// optional parameters
	int   N_symbols     = 0;
	int   n_frames      = 1;
	int   osf           = 2;
	float rolloff       = 0.2;
	int   grp_delay     = 20;

	// deduced parameters

	// ---------------------------------------------------------------------------------------------------- METHODS
	explicit Shaping_filter(const std::string &p = Shaping_filter_prefix);
	virtual ~Shaping_filter() = default;
	Shaping_filter* clone() const;

	// parameters construction
	virtual void get_description(cli::Argument_map_info &args) const;
	virtual void store          (const cli::Argument_map_value &vals);
	virtual void get_headers    (std::map<std::string,header_list>& headers, const bool full = true) const;

	template <typename R=float>
	module::Filter_UPRRC_ccr_naive<R>* build_shaping_flt() const;

	template <typename R=float>
	module::Filter_RRC_ccr_naive<R>*   build_matched_flt() const;
};
}
}

#endif /* FACTORY_SHAPING_FILTERP_ */


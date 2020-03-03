#ifndef FACTORY_SYNCHRONIZER_FREQUENCY_FINE_HPP_
#define FACTORY_SYNCHRONIZER_FREQUENCY_FINE_HPP_

#include <string>
#include <memory>
#include <aff3ct.hpp>

#include "Module/Synchronizer/Synchronizer_freq/Synchronizer_freq_fine/Synchronizer_freq_fine.hpp"

namespace aff3ct
{
namespace factory
{
extern const std::string Synchronizer_freq_fine_name;
extern const std::string Synchronizer_freq_fine_prefix;
struct Synchronizer_freq_fine : Factory
{
public:
	// ------------------------------------------------------------------------------------------------- PARAMETERS
	// required parameters

	// optional parameters
	int   N          = 0;
	int   n_frames   = 1;
	float lr_alpha   = 0.999;
	std::string type = "NORMAL";

	// deduced parameters

	// ---------------------------------------------------------------------------------------------------- METHODS
	explicit Synchronizer_freq_fine(const std::string &p = Synchronizer_freq_fine_prefix);
	virtual ~Synchronizer_freq_fine() = default;
	Synchronizer_freq_fine* clone() const;

	// parameters construction
	virtual void get_description(cli::Argument_map_info &args) const;
	virtual void store          (const cli::Argument_map_value &vals);
	virtual void get_headers    (std::map<std::string,header_list>& headers, const bool full = true) const;


	template <typename R = float>
	module::Synchronizer_freq_fine<R>* build_lr() const;

	template <typename R = float>
	module::Synchronizer_freq_fine<R>* build_freq_phase() const;

};
}
}

#endif /* FACTORY_SYNCHRONIZER_FREQUENCY_FINE_HPP_ */


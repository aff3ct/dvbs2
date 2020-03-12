#ifndef FACTORY_SYNCHRONIZER_FRAME_HPP_
#define FACTORY_SYNCHRONIZER_FRAME_HPP_

#include <string>
#include <memory>
#include <aff3ct.hpp>

#include "Module/Synchronizer/Synchronizer_frame/Synchronizer_frame.hpp"

namespace aff3ct
{
namespace factory
{
extern const std::string Synchronizer_frame_name;
extern const std::string Synchronizer_frame_prefix;
struct Synchronizer_frame : Factory
{
public:
	// ------------------------------------------------------------------------------------------------- PARAMETERS
	// required parameters

	// optional parameters
	int   N                    = 0;
	int   n_frames             = 1;

	float trigger              = (float)30;
	float alpha                = (float)0.9;
	float ref_delay            = (float)2.0;

	std::string type           = "FAST";

	// deduced parameters
	// ---------------------------------------------------------------------------------------------------- METHODS
	explicit Synchronizer_frame(const std::string &p = Synchronizer_frame_prefix);
	virtual ~Synchronizer_frame() = default;
	Synchronizer_frame* clone() const;

	// parameters construction
	virtual void get_description(cli::Argument_map_info &args) const;
	virtual void store          (const cli::Argument_map_value &vals);
	virtual void get_headers    (std::map<std::string,header_list>& headers, const bool full = true) const;


	template <typename R=float>
	module::Synchronizer_frame<R>* build() const;

};
}
}

#endif /* FACTORY_SYNCHRONIZER_FRAME_HPP_ */


#include "Factory/Module/Synchronizer_frame/Synchronizer_frame.hpp"

#include "Module/Synchronizer/Synchronizer_frame/Synchronizer_frame_DVBS2_aib.hpp"
#include "Module/Synchronizer/Synchronizer_frame/Synchronizer_frame_DVBS2_fast.hpp"
#include "Module/Synchronizer/Synchronizer_frame/Synchronizer_frame_perfect.hpp"

using namespace aff3ct;
using namespace aff3ct::factory;

const std::string aff3ct::factory::Synchronizer_frame_name   = "Synchronizer_frame";
const std::string aff3ct::factory::Synchronizer_frame_prefix = "sfm";

Synchronizer_frame
::Synchronizer_frame(const std::string &prefix)
: Factory(Synchronizer_frame_name, Synchronizer_frame_name, prefix)
{
}

Synchronizer_frame* Synchronizer_frame
::clone() const
{
	return new Synchronizer_frame(*this);
}

void Synchronizer_frame
::get_description(cli::Argument_map_info &args) const
{
	auto p = this->get_prefix();
	auto sfm_type_format = cli::Text(cli::Including_set("NORMAL", "PERFECT", "FAST"     ));

	args.add({p+"-type"   }, sfm_type_format, "Type of timing synchronization."          );
	args.add({p+"-alpha"  }, cli::Real(),     "Damping factor for frame synchronization.");
	args.add({p+"-trigger"}, cli::Real(),     "Trigger value to detect signal presence." );
	args.add({"perfect-sync"}, cli::None(),   "Enable genie aided synchronization."      );
}

void Synchronizer_frame
::store(const cli::Argument_map_value &vals)
{
	auto p = this->get_prefix();
	if (vals.exist({p+"-type"   })) this->type    = vals.at       ({p+"-type"   });
	if (vals.exist({p+"-alpha"  })) this->alpha   = vals.to_float ({p+"-alpha"  });
	if (vals.exist({p+"-trigger"})) this->trigger = vals.to_float ({p+"-trigger"});
    if (vals.exist({"perfect-sync" })) this->type = "PERFECT";

}

void Synchronizer_frame
::get_headers(std::map<std::string,tools::header_list>& headers, const bool full) const
{
	auto p = this->get_prefix();

	headers[p].push_back(std::make_pair("N. complex samples", std::to_string(this->N      )));
	headers[p].push_back(std::make_pair("Type              ", this->type                   ));
	if (this->type != "PERFECT")
	{
		headers[p].push_back(std::make_pair("Alpha             ", std::to_string(this->alpha  )));
		headers[p].push_back(std::make_pair("Trigger           ", std::to_string(this->trigger)));
	}
}

template <typename R>
module::Synchronizer_frame<R>* Synchronizer_frame
::build() const
{
	if (this->type == "PERFECT")
	{
		return dynamic_cast<module::Synchronizer_frame<R>*>(new module::Synchronizer_frame_perfect<R>(2 * this->N, this->ref_delay, this->n_frames));
		throw tools::cannot_allocate(__FILE__, __LINE__, __func__, "Wrong Synchronizer_frame type.");
	}
	else if (this->type == "FAST")
		return dynamic_cast<module::Synchronizer_frame<R>*>(new module::Synchronizer_frame_DVBS2_fast<R>(2 * this->N, this->alpha, this->trigger, this->n_frames));

	else if (this->type == "NORMAL")
		return dynamic_cast<module::Synchronizer_frame<R>*>(new module::Synchronizer_frame_DVBS2_aib <R>(2 * this->N, this->alpha, this->trigger, this->n_frames));

	else
		throw tools::cannot_allocate(__FILE__, __LINE__, __func__, "Wrong Synchronizer_frame type.");
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
template aff3ct::module::Synchronizer_frame<double>* aff3ct::factory::Synchronizer_frame::build<double>() const;
template aff3ct::module::Synchronizer_frame<float>*  aff3ct::factory::Synchronizer_frame::build<float >() const;
// ==================================================================================== explicit template instantiation

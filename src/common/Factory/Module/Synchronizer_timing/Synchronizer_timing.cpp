#include "Factory/Module/Synchronizer_timing/Synchronizer_timing.hpp"

#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_Gardner_aib.hpp"
#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_Gardner_fast.hpp"
#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_Gardner_ultra_osf2.hpp"
#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_Gardner_fast_osf2.hpp"
#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_timing_perfect.hpp"

using namespace aff3ct;
using namespace aff3ct::factory;

const std::string aff3ct::factory::Synchronizer_timing_name   = "Synchronizer_timing";
const std::string aff3ct::factory::Synchronizer_timing_prefix = "stm";

Synchronizer_timing
::Synchronizer_timing(const std::string &prefix)
: Factory(Synchronizer_timing_name, Synchronizer_timing_name, prefix)
{
}

Synchronizer_timing* Synchronizer_timing
::clone() const
{
	return new Synchronizer_timing(*this);
}

void Synchronizer_timing
::get_description(cli::Argument_map_info &args) const
{
	auto p = this->get_prefix();
	auto stm_type_format = cli::Text(cli::Including_set("NORMAL", "PERFECT", "FAST", "ULTRA"             ));

	args.add({p+"-type"},      stm_type_format, "Type of timing synchronization."                         );
	args.add({p+"-df"},        cli::Real(), "Damping factor of loop filter in Gardner's algorithm."       );
	args.add({p+"-nbw"},       cli::Real(), "Normalized Bandwidth of loop filter in Gardner's algorithm." );
	args.add({p+"-dg"},        cli::Real(), "Detector gain of loop filter in Gardner's algorithm."        );
	args.add({p+"-hold-size"}, cli::Integer(cli::Positive(), cli::Non_zero()),    "Gardner holding size." );


}

void Synchronizer_timing
::store(const cli::Argument_map_value &vals)
{
	auto p = this->get_prefix();
	if (vals.exist({p+"-type"     })) this->type                 = vals.at       ({p+"-type"      });
	if (vals.exist({p+"-df"       })) this->damping_factor       = vals.to_float ({p+"-df"        });
	if (vals.exist({p+"-nbw"      })) this->normalized_bandwidth = vals.to_float ({p+"-nbw"       });
	if (vals.exist({p+"-dg"       })) this->detector_gain        = vals.to_float ({p+"-dg"        });
	if (vals.exist({p+"-hold-size"})) this->hold_size            = vals.to_int   ({p+"-hold-size" });
}

void Synchronizer_timing
::get_headers(std::map<std::string,header_list>& headers, const bool full) const
{
	auto p = this->get_prefix();

	headers[p].push_back(std::make_pair("N. complex samples  ", std::to_string(this->N                   )));
	headers[p].push_back(std::make_pair("Type                ", this->type                                ));
	headers[p].push_back(std::make_pair("Damping Factor      ", std::to_string(this->damping_factor      )));
	headers[p].push_back(std::make_pair("Normalized Bandwidth", std::to_string(this->normalized_bandwidth)));
	headers[p].push_back(std::make_pair("Detector Gain       ", std::to_string(this->detector_gain       )));
	if (this->type == "ULTRA")
		headers[p].push_back(std::make_pair("Hold size       ", std::to_string(this->hold_size           )));
}

template <typename B, typename R>
module::Synchronizer_timing<B,R>* Synchronizer_timing
::build() const
{
	module::Synchronizer_timing<B,R>* sync_timing;
	if (this->type == "NORMAL")
	{
		sync_timing = dynamic_cast<module::Synchronizer_timing<B,R>*>(new module::Synchronizer_Gardner_aib<B,R>(2*this->N,
		                                                                                                        this->osf,
		                                                                                                        this->damping_factor,
		                                                                                                        this->normalized_bandwidth,
		                                                                                                        this->detector_gain,
		                                                                                                        this->n_frames));
	}
	else if (this->type == "PERFECT")
	{
		sync_timing = dynamic_cast<module::Synchronizer_timing<B,R>*>(new module::Synchronizer_timing_perfect<B,R>(2*this->N,
		                                                                                                           this->osf,
		                                                                                                           this->ref_delay,
		                                                                                                           this->n_frames));
	}
	else if(this->type == "FAST")
	{
		if (this->osf == 2)
			sync_timing = dynamic_cast<module::Synchronizer_timing<B,R>*>(new module::Synchronizer_Gardner_fast_osf2<B,R>(2*this->N,
		                                                                                                                  this->damping_factor,
		                                                                                                                  this->normalized_bandwidth,
		                                                                                                                  this->detector_gain,
		                                                                                                                  this->n_frames));
		else
			sync_timing = dynamic_cast<module::Synchronizer_timing<B,R>*>(new module::Synchronizer_Gardner_fast<B,R>(2*this->N,
		                                                                                                             this->osf,
		                                                                                                             this->damping_factor,
		                                                                                                             this->normalized_bandwidth,
		                                                                                                             this->detector_gain,
		                                                                                                             this->n_frames));
	}
	else if(this->type == "ULTRA")
	{
		if (this->osf == 2)
			sync_timing = dynamic_cast<module::Synchronizer_timing<B,R>*>(new module::Synchronizer_Gardner_ultra_osf2<B,R>(2*this->N,
		                                                                                                                   this->hold_size,
		                                                                                                                   this->damping_factor,
		                                                                                                                   this->normalized_bandwidth,
		                                                                                                                   this->detector_gain,
		                                                                                                                   this->n_frames));
		else
			throw tools::cannot_allocate(__FILE__, __LINE__, __func__, "Wrong Synchronizer_timing type.");
	}
	else
	{
		throw tools::cannot_allocate(__FILE__, __LINE__, __func__, "Wrong Synchronizer_timing type.");
	}
	return sync_timing;
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
template aff3ct::module::Synchronizer_timing<int,double>* aff3ct::factory::Synchronizer_timing::build<int,double>() const;
template aff3ct::module::Synchronizer_timing<int,float>*  aff3ct::factory::Synchronizer_timing::build<int,float >() const;
// ==================================================================================== explicit template instantiation

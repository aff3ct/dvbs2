#include "Factory/Module/Synchronizer_freq_coarse/Synchronizer_freq_coarse.hpp"

#include "Module/Synchronizer/Synchronizer_freq/Synchronizer_freq_coarse/Synchronizer_freq_coarse_DVBS2_aib.hpp"
#include "Module/Synchronizer/Synchronizer_freq/Synchronizer_freq_coarse/Synchronizer_freq_coarse_perfect.hpp"

using namespace aff3ct;
using namespace aff3ct::factory;

const std::string aff3ct::factory::Synchronizer_freq_coarse_name   = "Synchronizer_freq_coarse";
const std::string aff3ct::factory::Synchronizer_freq_coarse_prefix = "sfc";

Synchronizer_freq_coarse
::Synchronizer_freq_coarse(const std::string &prefix)
: Factory(Synchronizer_freq_coarse_name, Synchronizer_freq_coarse_name, prefix)
{
}

Synchronizer_freq_coarse* Synchronizer_freq_coarse
::clone() const
{
	return new Synchronizer_freq_coarse(*this);
}

void Synchronizer_freq_coarse
::get_description(cli::Argument_map_info &args) const
{
	auto p = this->get_prefix();
	auto sfc_type_format = cli::Text(cli::Including_set("NORMAL", "PERFECT" ));

	args.add({p+"-fra-size"},  cli::Integer(cli::Positive(), cli::Non_zero())      , ""                   );
	args.add({p+"-type"},      sfc_type_format, "Type of timing synchronization."                         );
	args.add({p+"-fra"},       cli::Integer(cli::Positive(), cli::Non_zero())      , ""                   );
	args.add({p+"-df"},        cli::Real(), "Damping factor of loop filter in coarse synchronizer."       );
	args.add({p+"-nbw"},       cli::Real(), "Normalized Bandwidth of loop filter in coarse synchronizer." );
	args.add({p+"-dg"},        cli::Real(), "Detector gain of loop filter in coarse synchronizer."        );
	args.add({"perfect-sync"}, cli::None(), "Enable genie aided synchronization."                         );

}

void Synchronizer_freq_coarse
::store(const cli::Argument_map_value &vals)
{
	auto p = this->get_prefix();
	if (vals.exist({p+"-type"      })) this->type                 = vals.at       ({p+"-type"      });
	if (vals.exist({p+"-fra-size"  })) this->N                    = vals.to_int   ({p+"-fra-size"});
	if (vals.exist({p+"-fra"       })) this->n_frames             = vals.to_int   ({p+"-fra"     });
	if (vals.exist({p+"-nbw"       })) this->normalized_bandwidth = vals.to_float ({p+"-nbw"     });
	if (vals.exist({"perfect-sync" })) this->type                 = "PERFECT";
}

void Synchronizer_freq_coarse
::get_headers(std::map<std::string,header_list>& headers, const bool full) const
{
	auto p = this->get_prefix();

	headers[p].push_back(std::make_pair("N. samples          ", std::to_string(this->N                   )));
	headers[p].push_back(std::make_pair("Type                ", this->type                                ));

	if (this->type != "PERFECT")
	{
		headers[p].push_back(std::make_pair("Damping Factor      ", std::to_string(this->damping_factor      )));
		headers[p].push_back(std::make_pair("Normalized Bandwidth", std::to_string(this->normalized_bandwidth)));
	}
}

template <typename R>
module::Synchronizer_freq_coarse<R>* Synchronizer_freq_coarse
::build() const
{

	if (this->type == "PERFECT")
		return dynamic_cast<module::Synchronizer_freq_coarse<R>*>(new module::Synchronizer_freq_coarse_perfect<R>(2 * this->N,
		                                                                                                          this->ref_freq_shift,
		                                                                                                          this->n_frames));

	else if(this->type == "NORMAL")
		return dynamic_cast<module::Synchronizer_freq_coarse<R>*>(new module::Synchronizer_freq_coarse_DVBS2_aib<R>(2 * this->N,
		                                                                                                            this->osf,
		                                                                                                            this->damping_factor,
		                                                                                                            this->normalized_bandwidth,
		                                                                                                            this->n_frames));
	else
		throw tools::cannot_allocate(__FILE__, __LINE__, __func__, "Wrong Synchronizer_coarse type.");

}


// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
template aff3ct::module::Synchronizer_freq_coarse<double>* aff3ct::factory::Synchronizer_freq_coarse::build<double>() const;
template aff3ct::module::Synchronizer_freq_coarse<float>*  aff3ct::factory::Synchronizer_freq_coarse::build<float >() const;
// ==================================================================================== explicit template instantiation

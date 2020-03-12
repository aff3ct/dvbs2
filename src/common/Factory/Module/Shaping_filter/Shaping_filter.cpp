#include "Factory/Module/Shaping_filter/Shaping_filter.hpp"


using namespace aff3ct;
using namespace aff3ct::factory;

const std::string aff3ct::factory::Shaping_filter_name   = "Shaping_filter";
const std::string aff3ct::factory::Shaping_filter_prefix = "shp";

Shaping_filter
::Shaping_filter(const std::string &prefix)
: Factory(Shaping_filter_name, Shaping_filter_name, prefix)
{
}

Shaping_filter* Shaping_filter
::clone() const
{
	return new Shaping_filter(*this);
}

void Shaping_filter
::get_description(cli::Argument_map_info &args) const
{
	auto p = this->get_prefix();

	args.add({p+"-grp-delay"}, cli::Integer(cli::Positive(), cli::Non_zero()), "RRC Group delay."        );
	args.add({p+"-rolloff"},   cli::Real(),                                    "RRC rolloff."            );
	args.add({p+"-osf"},       cli::Integer(cli::Positive(), cli::Non_zero()), "RRC oversampling factor.");

}

void Shaping_filter
::store(const cli::Argument_map_value &vals)
{
	auto p = this->get_prefix();
	if (vals.exist({p+"-grp-delay"})) this->grp_delay = vals.to_int  ({p+"-grp-delay"});
	if (vals.exist({p+"-rolloff" } )) this->rolloff   = vals.to_float({p+"-rolloff"}  );
	if (vals.exist({p+"-osf"     } )) this->osf       = vals.to_int  ({p+"-osf"}      );

}

void Shaping_filter
::get_headers(std::map<std::string,header_list>& headers, const bool full) const
{
	auto p = this->get_prefix();

	headers[p].push_back(std::make_pair("N. complex samples  ", std::to_string(this->N_symbols * this->osf)));
	headers[p].push_back(std::make_pair("N. complex symbols  ", std::to_string(this->N_symbols            )));
	headers[p].push_back(std::make_pair("Oversampling Factor ", std::to_string(this->osf                  )));
	headers[p].push_back(std::make_pair("Rolloff Factor      ", std::to_string(this->rolloff              )));
	headers[p].push_back(std::make_pair("Group Delay         ", std::to_string(this->grp_delay            )));
}

template <typename R>
module::Filter_UPRRC_ccr_naive<R>* Shaping_filter
::build_shaping_flt() const
{
	return new module::Filter_UPRRC_ccr_naive<R>(this->N_symbols * 2,
	                                             this->rolloff,
	                                             this->osf,
	                                             this->grp_delay,
	                                             this->n_frames);
}

template <typename R>
module::Filter_RRC_ccr_naive<R>* Shaping_filter
::build_matched_flt() const
{
	return new module::Filter_RRC_ccr_naive<R>(this->N_symbols * this->osf * 2,
	                                           this->rolloff,
	                                           this->osf,
	                                           this->grp_delay,
	                                           this->n_frames);
}
// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
template aff3ct::module::Filter_RRC_ccr_naive<double>*   aff3ct::factory::Shaping_filter::build_matched_flt<double>() const;
template aff3ct::module::Filter_RRC_ccr_naive<float>*    aff3ct::factory::Shaping_filter::build_matched_flt<float >() const;
template aff3ct::module::Filter_UPRRC_ccr_naive<double>* aff3ct::factory::Shaping_filter::build_shaping_flt<double>() const;
template aff3ct::module::Filter_UPRRC_ccr_naive<float>*  aff3ct::factory::Shaping_filter::build_shaping_flt<float >() const;
// ==================================================================================== explicit template instantiation

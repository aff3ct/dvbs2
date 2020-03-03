#include "Factory/Module/Synchronizer_freq_fine/Synchronizer_freq_fine.hpp"
#include "Module/Synchronizer/Synchronizer_freq/Synchronizer_freq_fine/Synchronizer_freq_phase_DVBS2_aib.hpp"
#include "Module/Synchronizer/Synchronizer_freq/Synchronizer_freq_fine/Synchronizer_Luise_Reggiannini_DVBS2_aib.hpp"
#include "Module/Synchronizer/Synchronizer_freq/Synchronizer_freq_fine/Synchronizer_freq_fine_perfect.hpp"

using namespace aff3ct;
using namespace aff3ct::factory;

const std::string aff3ct::factory::Synchronizer_freq_fine_name   = "Synchronizer_freq_fine";
const std::string aff3ct::factory::Synchronizer_freq_fine_prefix = "sff";

Synchronizer_freq_fine
::Synchronizer_freq_fine(const std::string &prefix)
: Factory(Synchronizer_freq_fine_name, Synchronizer_freq_fine_name, prefix)
{
}

Synchronizer_freq_fine* Synchronizer_freq_fine
::clone() const
{
	return new Synchronizer_freq_fine(*this);
}

void Synchronizer_freq_fine
::get_description(cli::Argument_map_info &args) const
{
	auto p = this->get_prefix();
	auto sff_type_format = cli::Text(cli::Including_set("NORMAL", "PERFECT" ));

	args.add({p+"-fra-size"},    cli::Integer(cli::Positive(), cli::Non_zero())          , "");
	args.add({p+"-lr-alpha"},     cli::Real()    , "Damping factor for the Luise and Reggiannini algorithm.");
	args.add({p+"-type"},         sff_type_format, "Type of timing synchronization."                        );
	args.add({p+"-fra"},         cli::Integer(cli::Positive(), cli::Non_zero())          , "");
}

void Synchronizer_freq_fine
::store(const cli::Argument_map_value &vals)
{
	auto p = this->get_prefix();
	if (vals.exist({p+"-fra-size"})) this->N        = vals.to_int   ({p+"-fra-size"});
	if (vals.exist({p+"-fra"     })) this->n_frames = vals.to_int   ({p+"-fra"     });
	if (vals.exist({p+"-lr-alpha"})) this->lr_alpha = vals.to_float ({p+"-lr-alpha"});
	if (vals.exist({p+"-type"    })) this->type     = vals.at       ({p+"-type"   });
}

void Synchronizer_freq_fine
::get_headers(std::map<std::string,header_list>& headers, const bool full) const
{
	auto p = this->get_prefix();

	headers[p].push_back(std::make_pair("N. samples", std::to_string(this->N       )));
	headers[p].push_back(std::make_pair("Type      ", this->type                    ));
	headers[p].push_back(std::make_pair("L&R Alpha ", std::to_string(this->lr_alpha)));
}

template <typename R>
module::Synchronizer_freq_fine<R>* Synchronizer_freq_fine
::build_lr() const
{
	if (this->type == "PERFECT")
		return dynamic_cast<module::Synchronizer_freq_fine<R>*>(new module::Synchronizer_freq_fine_perfect<R>(2 * this->N, (R)0, (R)0, this->n_frames));
	else if (this->type == "NORMAL")
		return dynamic_cast<module::Synchronizer_freq_fine<R>*>(new module::Synchronizer_Luise_Reggiannini_DVBS2_aib<R>(2 * this->N, this->lr_alpha, this->n_frames));

	throw tools::cannot_allocate(__FILE__, __LINE__, __func__);
}

template <typename R>
module::Synchronizer_freq_fine<R>* Synchronizer_freq_fine
::build_freq_phase() const
{
	if (this->type == "PERFECT")
		return dynamic_cast<module::Synchronizer_freq_fine<R>*>(new module::Synchronizer_freq_fine_perfect<R>(2 * this->N, (R)0, (R)0, this->n_frames));
	else if (this->type == "NORMAL")
		return dynamic_cast<module::Synchronizer_freq_fine<R>*>(new module::Synchronizer_freq_phase_DVBS2_aib<R>(2 * this->N, this->n_frames));

	throw tools::cannot_allocate(__FILE__, __LINE__, __func__);
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
template aff3ct::module::Synchronizer_freq_fine<double>* aff3ct::factory::Synchronizer_freq_fine::build_lr<double>() const;
template aff3ct::module::Synchronizer_freq_fine<float>*  aff3ct::factory::Synchronizer_freq_fine::build_lr<float >() const;
template aff3ct::module::Synchronizer_freq_fine<double>* aff3ct::factory::Synchronizer_freq_fine::build_freq_phase<double>() const;
template aff3ct::module::Synchronizer_freq_fine<float>*  aff3ct::factory::Synchronizer_freq_fine::build_freq_phase<float >() const;
// ==================================================================================== explicit template instantiation

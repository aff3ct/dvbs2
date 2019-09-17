#include "Factory/Module/Radio/Radio_USRP/Radio_USRP.hpp"

using namespace aff3ct;
using namespace aff3ct::factory;

const std::string aff3ct::factory::Radio_USRP_name   = "Radio_USRP";
const std::string aff3ct::factory::Radio_USRP_prefix = "rad";

Radio_USRP::parameters
::parameters(const std::string &prefix)
: Factory::parameters(Radio_USRP_name, Radio_USRP_name, prefix)
{
}

Radio_USRP::parameters* Radio_USRP::parameters
::clone() const
{
	return new Radio_USRP::parameters(*this);
}

void Radio_USRP::parameters
::get_description(tools::Argument_map_info &args) const
{
	auto p = this->get_prefix();
	const std::string class_name = "factory::Radio_USRP::parameters::";

	tools::add_arg(args, p, class_name+"p+fra-size,N",
	               tools::Integer(tools::Positive(), tools::Non_zero()));	

	tools::add_arg(args, p, class_name+"p+clk-rate",
		tools::Real(tools::Positive(), tools::Non_zero()));

	tools::add_arg(args, p, class_name+"p+rx_subdev_spec,",
		tools::Text());

	tools::add_arg(args, p, class_name+"p+rx-rate,",
		tools::Real(tools::Positive(), tools::Non_zero()));

	tools::add_arg(args, p, class_name+"p+rx-freq,",
		tools::Real(tools::Positive(), tools::Non_zero()));

	tools::add_arg(args, p, class_name+"p+rx-gain,",
		tools::Real(tools::Positive(), tools::Non_zero()));

	tools::add_arg(args, p, class_name+"p+tx_subdev_spec,",
		tools::Text());

	tools::add_arg(args, p, class_name+"p+tx-rate,",
		tools::Real(tools::Positive(), tools::Non_zero()));

	tools::add_arg(args, p, class_name+"p+tx-freq,",
		tools::Real(tools::Positive(), tools::Non_zero()));

	tools::add_arg(args, p, class_name+"p+tx-gain,",
		tools::Real(tools::Positive(), tools::Non_zero()));

}

void Radio_USRP::parameters
::store(const tools::Argument_map_value &vals)
{
	// auto p = this->get_prefix();

	// if(vals.exist({p+"-info-bits",  "K"})) this->K        = vals.to_int({p+"-info-bits",  "K"});
	// if(vals.exist({p+"-fra",        "F"})) this->n_frames = vals.to_int({p+"-fra",        "F"});
	// if(vals.exist({p+"-type", p+"-poly"})) this->type     = vals.at    ({p+"-type", p+"-poly"});
	// if(vals.exist({p+"-implem"         })) this->implem   = vals.at    ({p+"-implem"         });
	// if(vals.exist({p+"-size"           })) this->size     = vals.to_int({p+"-size"           });

	// if (this->type != "NO" && !this->type.empty() && !this->size)
	// 	this->size = module::CRC_polynomial<B>::get_size(this->type);
}

void Radio_USRP::parameters
::get_headers(std::map<std::string,header_list>& headers, const bool full) const
{
	// auto p = this->get_prefix();

	// if (this->type != "NO" && !this->type.empty())
	// {
	// 	auto poly_name = module::CRC_polynomial<B>::get_name(this->type);
	// 	if (!poly_name.empty())
	// 		headers[p].push_back(std::make_pair("Type", poly_name));
	// 	else
	// 	{
	// 		std::stringstream poly_val;
	// 		poly_val << "0x" << std::hex << module::CRC_polynomial<B>::get_value(this->type);
	// 		headers[p].push_back(std::make_pair("Type", poly_val.str()));
	// 	}
	// 	std::stringstream poly_val;
	// 	poly_val << "0x" << std::hex << module::CRC_polynomial<B>::get_value(this->type);
	// 	headers[p].push_back(std::make_pair("Polynomial (hexadecimal)", poly_val.str()));

	// 	auto poly_size = module::CRC_polynomial<B>::get_size(this->type);
	// 	headers[p].push_back(std::make_pair("Size (in bit)", std::to_string(poly_size ? poly_size : this->size)));
	// }
	// else
	// 	headers[p].push_back(std::make_pair("Type", "NO"));

	// headers[p].push_back(std::make_pair("Implementation", this->implem));

	// if (full) headers[p].push_back(std::make_pair("Info. bits (K)", std::to_string(this->K)));
	// if (full) headers[p].push_back(std::make_pair("Inter frame level", std::to_string(this->n_frames)));
}

template <typename D>
module::Radio<D>* Radio_USRP::parameters
::build() const
{

	// return new module::CRC_polynomial      <B>(K, poly, size, n_frames);

	throw tools::cannot_allocate(__FILE__, __LINE__, __func__);
}

template <typename D>
module::Radio<D>* Radio_USRP
::build(const parameters &params)
{
	return params.template build<D>();
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
template aff3ct::module::Radio<double>* aff3ct::factory::Radio_USRP::parameters::build<double>() const;
template aff3ct::module::Radio<float>*  aff3ct::factory::Radio_USRP::parameters::build<float >() const;
template aff3ct::module::Radio<double>* aff3ct::factory::Radio_USRP::build<double>(const aff3ct::factory::Radio_USRP::parameters&);
template aff3ct::module::Radio<float>*  aff3ct::factory::Radio_USRP::build<float >(const aff3ct::factory::Radio_USRP::parameters&);
// ==================================================================================== explicit template instantiation

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

	tools::add_arg(args, p, class_name+"p+fra,F",
		tools::Integer(tools::Positive(), tools::Non_zero()));

	tools::add_arg(args, p, class_name+"p+clk-rate",
		tools::Real(tools::Positive(), tools::Non_zero()));

	tools::add_arg(args, p, class_name+"p+rx-subdev-spec",
		tools::Text());

	tools::add_arg(args, p, class_name+"p+rx-rate",
		tools::Real(tools::Positive(), tools::Non_zero()));

	tools::add_arg(args, p, class_name+"p+rx-freq",
		tools::Real(tools::Positive(), tools::Non_zero()));

	tools::add_arg(args, p, class_name+"p+rx-gain",
		tools::Real(tools::Positive(), tools::Non_zero()));

	tools::add_arg(args, p, class_name+"p+tx-subdev-spec",
		tools::Text());

	tools::add_arg(args, p, class_name+"p+tx-rate",
		tools::Real(tools::Positive(), tools::Non_zero()));

	tools::add_arg(args, p, class_name+"p+tx-freq",
		tools::Real(tools::Positive(), tools::Non_zero()));

	tools::add_arg(args, p, class_name+"p+tx-gain",
		tools::Real(tools::Positive(), tools::Non_zero()));

	tools::add_arg(args, p, class_name+"p+ip-addr",
		tools::Text());
}

void Radio_USRP::parameters
::store(const tools::Argument_map_value &vals)
{
	auto p = this->get_prefix();

	if(vals.exist({p+"-fra-size",  "N"})) this->N              = vals.to_int  ({p+"-fra-size",  "N"});
	if(vals.exist({p+"-clk-rate"      })) this->clk_rate       = vals.to_float({p+"-clk-rate"      });
	if(vals.exist({p+"-rx-subdev-spec"})) this->rx_subdev_spec = vals.at      ({p+"-rx-subdev-spec"});
	if(vals.exist({p+"-rx-rate"       })) this->rx_rate        = vals.to_float({p+"-rx-rate"       });
	if(vals.exist({p+"-rx-freq"       })) this->rx_freq        = vals.to_float({p+"-rx-freq"       });
	if(vals.exist({p+"-rx-gain"       })) this->rx_gain        = vals.to_float({p+"-rx-gain"       });
	if(vals.exist({p+"-tx-subdev-spec"})) this->tx_subdev_spec = vals.at      ({p+"-tx-subdev-spec"});
	if(vals.exist({p+"-tx-rate"       })) this->tx_rate        = vals.to_float({p+"-tx-rate"       });
	if(vals.exist({p+"-tx-freq"       })) this->tx_freq        = vals.to_float({p+"-tx-freq"       });
	if(vals.exist({p+"-tx-gain"       })) this->tx_gain        = vals.to_float({p+"-tx-gain"       });
	if(vals.exist({p+"-ip-addr"       })) this->usrp_addr      = vals.at      ({p+"-ip-addr"       });
	if(vals.exist({p+"-fra",       "F"})) this->n_frames       = vals.to_int  ({p+"-fra",       "F"});
}

void Radio_USRP::parameters
::get_headers(std::map<std::string,header_list>& headers, const bool full) const
{
	auto p = this->get_prefix();

	headers[p].push_back(std::make_pair("N. cw  (N)", std::to_string(this->N)));
	headers[p].push_back(std::make_pair("Clk rate  ", std::to_string(this->clk_rate)));
	headers[p].push_back(std::make_pair("Rx rate   ", std::to_string(this->rx_rate)));
	headers[p].push_back(std::make_pair("Rx freq   ", std::to_string(this->rx_freq)));
	headers[p].push_back(std::make_pair("Rx gain   ", std::to_string(this->rx_gain)));
	headers[p].push_back(std::make_pair("Tx rate   ", std::to_string(this->tx_rate)));
	headers[p].push_back(std::make_pair("Tx freq   ", std::to_string(this->tx_freq)));
	headers[p].push_back(std::make_pair("Tx gain   ", std::to_string(this->tx_gain)));
}

template <typename D>
module::Radio<D>* Radio_USRP::parameters
::build() const
{

	return new module::Radio_USRP<D> (this->N, this->usrp_addr, this->clk_rate, this->rx_rate, this->rx_freq,
	                                  this->rx_subdev_spec,this->tx_rate, this->tx_freq, this->tx_subdev_spec,
	                                  this->n_frames);
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

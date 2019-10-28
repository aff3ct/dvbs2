#include "Factory/Module/Radio/Radio.hpp"

#ifdef DVBS2O_LINK_UHD
	#include "Module/Radio/Radio_USRP/Radio_USRP.hpp"
#endif

using namespace aff3ct;
using namespace aff3ct::factory;

const std::string aff3ct::factory::Radio_name   = "Radio";
const std::string aff3ct::factory::Radio_prefix = "rad";

Radio::parameters
::parameters(const std::string &prefix)
: Factory::parameters(Radio_name, Radio_name, prefix)
{
}

Radio::parameters* Radio::parameters
::clone() const
{
	return new Radio::parameters(*this);
}

void Radio::parameters
::get_description(cli::Argument_map_info &args) const
{
	auto p = this->get_prefix();
	const std::string class_name = "factory::Radio::parameters::";

	tools::add_arg(args, p, class_name+"p+fra-size,N",
	               cli::Integer(cli::Positive(), cli::Non_zero()));

	tools::add_arg(args, p, class_name+"p+fra,F",
		cli::Integer(cli::Positive(), cli::Non_zero()));

	tools::add_arg(args, p, class_name+"p+clk-rate",
		cli::Real(cli::Positive(), cli::Non_zero()));

	tools::add_arg(args, p, class_name+"p+rx-subdev-spec",
		cli::Text());

	tools::add_arg(args, p, class_name+"p+rx-ant",
		cli::Text());

	tools::add_arg(args, p, class_name+"p+rx-rate",
		cli::Real(cli::Positive(), cli::Non_zero()));

	tools::add_arg(args, p, class_name+"p+rx-freq",
		cli::Real(cli::Positive(), cli::Non_zero()));

	tools::add_arg(args, p, class_name+"p+rx-gain",
		cli::Real(cli::Positive(), cli::Non_zero()));

	tools::add_arg(args, p, class_name+"p+tx-subdev-spec",
		cli::Text());

	tools::add_arg(args, p, class_name+"p+tx-ant",
		cli::Text());

	tools::add_arg(args, p, class_name+"p+tx-rate",
		cli::Real(cli::Positive(), cli::Non_zero()));

	tools::add_arg(args, p, class_name+"p+tx-freq",
		cli::Real(cli::Positive(), cli::Non_zero()));

	tools::add_arg(args, p, class_name+"p+tx-gain",
		cli::Real(cli::Positive(), cli::Non_zero()));

	tools::add_arg(args, p, class_name+"p+ip-addr",
		cli::Text());
}

void Radio::parameters
::store(const cli::Argument_map_value &vals)
{
	auto p = this->get_prefix();

	if(vals.exist({p+"-fra-size",  "N"})) this->N              = vals.to_int  ({p+"-fra-size",  "N"});
	if(vals.exist({p+"-clk-rate"      })) this->clk_rate       = vals.to_float({p+"-clk-rate"      });
	if(vals.exist({p+"-rx-subdev-spec"})) this->rx_subdev_spec = vals.at      ({p+"-rx-subdev-spec"});
	if(vals.exist({p+"-rx-ant"        })) this->rx_antenna     = vals.at      ({p+"-rx-ant"});
	if(vals.exist({p+"-rx-rate"       })) this->rx_rate        = vals.to_float({p+"-rx-rate"       });
	if(vals.exist({p+"-rx-freq"       })) this->rx_freq        = vals.to_float({p+"-rx-freq"       });
	if(vals.exist({p+"-rx-gain"       })) this->rx_gain        = vals.to_float({p+"-rx-gain"       });
	if(vals.exist({p+"-tx-subdev-spec"})) this->tx_subdev_spec = vals.at      ({p+"-tx-subdev-spec"});
	if(vals.exist({p+"-Tx-ant"        })) this->tx_antenna     = vals.at      ({p+"-tx-ant"});
	if(vals.exist({p+"-tx-rate"       })) this->tx_rate        = vals.to_float({p+"-tx-rate"       });
	if(vals.exist({p+"-tx-freq"       })) this->tx_freq        = vals.to_float({p+"-tx-freq"       });
	if(vals.exist({p+"-tx-gain"       })) this->tx_gain        = vals.to_float({p+"-tx-gain"       });
	if(vals.exist({p+"-ip-addr"       })) this->usrp_addr      = vals.at      ({p+"-ip-addr"       });
	if(vals.exist({p+"-fra",       "F"})) this->n_frames       = vals.to_int  ({p+"-fra",       "F"});
}

void Radio::parameters
::get_headers(std::map<std::string,header_list>& headers, const bool full) const
{
	auto p = this->get_prefix();

	headers[p].push_back(std::make_pair("N. cw  (N)", std::to_string(this->N)));
	headers[p].push_back(std::make_pair("Clk rate  ", std::to_string(this->clk_rate)));
	headers[p].push_back(std::make_pair("Rx rate   ", std::to_string(this->rx_rate)));
	headers[p].push_back(std::make_pair("Rx subdev ", this->rx_subdev_spec));
	headers[p].push_back(std::make_pair("Rx antenna", this->rx_antenna));
	headers[p].push_back(std::make_pair("Rx freq   ", std::to_string(this->rx_freq)));
	headers[p].push_back(std::make_pair("Rx gain   ", std::to_string(this->rx_gain)));
	headers[p].push_back(std::make_pair("Tx subdev ", this->tx_subdev_spec));
	headers[p].push_back(std::make_pair("Tx antenna", this->tx_antenna));
	headers[p].push_back(std::make_pair("Tx rate   ", std::to_string(this->tx_rate)));
	headers[p].push_back(std::make_pair("Tx rate   ", std::to_string(this->tx_rate)));
	headers[p].push_back(std::make_pair("Tx rate   ", std::to_string(this->tx_rate)));
	headers[p].push_back(std::make_pair("Tx freq   ", std::to_string(this->tx_freq)));
	headers[p].push_back(std::make_pair("Tx gain   ", std::to_string(this->tx_gain)));
}

template <typename R>
module::Radio<R>* Radio::parameters
::build() const
{
	#ifdef DVBS2O_LINK_UHD
	return new module::Radio_USRP<R> (this->N, this->usrp_addr, this->clk_rate, this->rx_rate, this->rx_freq,
	                                  this->rx_subdev_spec, this->rx_antenna, this->tx_rate, this->tx_freq, this->tx_subdev_spec,
	                                  this->tx_antenna, this->n_frames, this->rx_gain, this->tx_gain);
	#endif

	throw tools::cannot_allocate(__FILE__, __LINE__, __func__, "A dependency is possibly missing (e.g. UHD).");
}

template <typename R>
module::Radio<R>* Radio
::build(const parameters &params)
{
	return params.template build<R>();
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
template aff3ct::module::Radio<double>*      aff3ct::factory::Radio::parameters::build<double     >() const;
template aff3ct::module::Radio<float>*       aff3ct::factory::Radio::parameters::build<float      >() const;
template aff3ct::module::Radio<short>*       aff3ct::factory::Radio::parameters::build<short      >() const;
template aff3ct::module::Radio<signed char>* aff3ct::factory::Radio::parameters::build<signed char>() const;
template aff3ct::module::Radio<double>*      aff3ct::factory::Radio::build<double     >(const aff3ct::factory::Radio::parameters&);
template aff3ct::module::Radio<float>*       aff3ct::factory::Radio::build<float      >(const aff3ct::factory::Radio::parameters&);
template aff3ct::module::Radio<short>*       aff3ct::factory::Radio::build<short      >(const aff3ct::factory::Radio::parameters&);
template aff3ct::module::Radio<signed char>* aff3ct::factory::Radio::build<signed char>(const aff3ct::factory::Radio::parameters&);
// ==================================================================================== explicit template instantiation

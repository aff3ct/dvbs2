#include "Factory/Module/Radio/Radio.hpp"
#include "Module/Radio/Radio_NO/Radio_NO.hpp"
#include "Module/Radio/Radio_user/Radio_user_binary.hpp"
#ifdef DVBS2_LINK_UHD
	#include "Module/Radio/Radio_USRP/Radio_USRP.hpp"
#endif

using namespace aff3ct;
using namespace aff3ct::factory;

const std::string aff3ct::factory::Radio_name   = "Radio";
const std::string aff3ct::factory::Radio_prefix = "rad";

Radio
::Radio(const std::string &prefix)
: Factory(Radio_name, Radio_name, prefix)
{
}

Radio* Radio
::clone() const
{
	return new Radio(*this);
}

void Radio
::get_description(cli::Argument_map_info &args) const
{
	auto p = this->get_prefix();
	args.add({p+"-fra-size",  "N"}, cli::Integer(cli::Positive(), cli::Non_zero())          , "");
	args.add({p+"-type"          }, cli::Text(cli::Including_set("USRP", "USER_BIN", "NO")) , "");
	args.add({p+"-threaded"      }, cli::None()                                             , "");
	args.add({p+"-fifo-size"     }, cli::Integer<uint64_t>(cli::Positive(), cli::Non_zero()), "");
	args.add({p+"-fra",       "F"}, cli::Integer(cli::Positive(), cli::Non_zero())          , "");
	args.add({p+"-clk-rate"      }, cli::Real(cli::Positive(), cli::Non_zero())             , "");
	args.add({p+"-rx-subdev-spec"}, cli::Text()                                             , "");
	args.add({p+"-rx-ant"        }, cli::Text()                                             , "");
	args.add({p+"-rx-rate"       }, cli::Real(cli::Positive(), cli::Non_zero())             , "");
	args.add({p+"-rx-freq"       }, cli::Real(cli::Positive(), cli::Non_zero())             , "");
	args.add({p+"-rx-gain"       }, cli::Real(cli::Positive(), cli::Non_zero())             , "");
	args.add({p+"-rx-file-path"  }, cli::Text()                                             , "");
	args.add({p+"-tx-file-path"  }, cli::Text()                                             , "");
	args.add({p+"-tx-subdev-spec"}, cli::Text()                                             , "");
	args.add({p+"-tx-ant"        }, cli::Text()                                             , "");
	args.add({p+"-tx-rate"       }, cli::Real(cli::Positive(), cli::Non_zero())             , "");
	args.add({p+"-tx-freq"       }, cli::Real(cli::Positive(), cli::Non_zero())             , "");
	args.add({p+"-tx-gain"       }, cli::Real(cli::Positive(), cli::Non_zero())             , "");
	args.add({p+"-ip-addr"       }, cli::Text()                                             , "");
}

void Radio
::store(const cli::Argument_map_value &vals)
{
	auto p = this->get_prefix();
	if (vals.exist({p+"-fra-size",  "N"})) this->N              = vals.to_int   ({p+"-fra-size",  "N"});
	if (vals.exist({p+"-type"          })) this->type           = vals.at       ({p+"-type"          });
	if (vals.exist({p+"-threaded"      })) this->threaded       = true                                 ;
	if (vals.exist({p+"-fifo-size"     })) this->fifo_size      = vals.to_uint64({p+"-fifo-size"     });
	if (vals.exist({p+"-fra",       "F"})) this->n_frames       = vals.to_int   ({p+"-fra",       "F"});
	if (vals.exist({p+"-clk-rate"      })) this->clk_rate       = vals.to_float ({p+"-clk-rate"      });
	if (vals.exist({p+"-rx-subdev-spec"})) this->rx_subdev_spec = vals.at       ({p+"-rx-subdev-spec"});
	if (vals.exist({p+"-rx-ant"        })) this->rx_antenna     = vals.at       ({p+"-rx-ant"        });
	if (vals.exist({p+"-rx-rate"       })) this->rx_enabled     = true                                 ;
	if (vals.exist({p+"-rx-rate"       })) this->rx_rate        = vals.to_float ({p+"-rx-rate"       });
	if (vals.exist({p+"-rx-freq"       })) this->rx_freq        = vals.to_float ({p+"-rx-freq"       });
	if (vals.exist({p+"-rx-gain"       })) this->rx_gain        = vals.to_float ({p+"-rx-gain"       });
	if (vals.exist({p+"-rx-file-path"  })) this->rx_filepath    = vals.at       ({p+"-rx-file-path"  });
	if (vals.exist({p+"-tx-file-path"  })) this->tx_filepath    = vals.at       ({p+"-tx-file-path"  });
	if (vals.exist({p+"-tx-subdev-spec"})) this->tx_subdev_spec = vals.at       ({p+"-tx-subdev-spec"});
	if (vals.exist({p+"-Tx-ant"        })) this->tx_antenna     = vals.at       ({p+"-tx-ant"        });
	if (vals.exist({p+"-tx-rate"       })) this->tx_enabled     = true                                 ;
	if (vals.exist({p+"-tx-rate"       })) this->tx_rate        = vals.to_float ({p+"-tx-rate"       });
	if (vals.exist({p+"-tx-freq"       })) this->tx_freq        = vals.to_float ({p+"-tx-freq"       });
	if (vals.exist({p+"-tx-gain"       })) this->tx_gain        = vals.to_float ({p+"-tx-gain"       });
	if (vals.exist({p+"-ip-addr"       })) this->usrp_addr      = vals.at       ({p+"-ip-addr"       });
}

void Radio
::get_headers(std::map<std::string,header_list>& headers, const bool full) const
{
	auto p = this->get_prefix();

	headers[p].push_back(std::make_pair("N. samples", std::to_string(this->N        )));
	headers[p].push_back(std::make_pair("Type      ", this->type                     ));
	headers[p].push_back(std::make_pair("Threaded  ", this->threaded ? "YES" : "NO"  ));
	headers[p].push_back(std::make_pair("Fifo size ", std::to_string(this->fifo_size)));
	headers[p].push_back(std::make_pair("Clk rate  ", std::to_string(this->clk_rate )));
	headers[p].push_back(std::make_pair("Rx rate   ", std::to_string(this->rx_rate  )));
	headers[p].push_back(std::make_pair("Rx subdev ", this->rx_subdev_spec           ));
	headers[p].push_back(std::make_pair("Rx antenna", this->rx_antenna               ));
	headers[p].push_back(std::make_pair("Rx freq   ", std::to_string(this->rx_freq  )));
	headers[p].push_back(std::make_pair("Rx gain   ", std::to_string(this->rx_gain  )));
	headers[p].push_back(std::make_pair("Rx File   ", this->rx_filepath              ));
	headers[p].push_back(std::make_pair("Tx File   ", this->tx_filepath              ));
	headers[p].push_back(std::make_pair("Tx subdev ", this->tx_subdev_spec           ));
	headers[p].push_back(std::make_pair("Tx antenna", this->tx_antenna               ));
	headers[p].push_back(std::make_pair("Tx rate   ", std::to_string(this->tx_rate  )));
	headers[p].push_back(std::make_pair("Tx rate   ", std::to_string(this->tx_rate  )));
	headers[p].push_back(std::make_pair("Tx rate   ", std::to_string(this->tx_rate  )));
	headers[p].push_back(std::make_pair("Tx freq   ", std::to_string(this->tx_freq  )));
	headers[p].push_back(std::make_pair("Tx gain   ", std::to_string(this->tx_gain  )));
}

template <typename R>
module::Radio<R>* Radio
::build() const
{
	if (this->type == "NO")
		return new module::Radio_NO<R>(this->N, this->n_frames);
	else if (this->type == "USER_BIN")
		return new module::Radio_user_binary<R>(this->N, this->rx_filepath, this->tx_filepath, this->n_frames);
	#ifdef DVBS2_LINK_UHD
	else if (this->type == "USRP")
		return new module::Radio_USRP<R>(*this);
	#endif

	throw tools::cannot_allocate(__FILE__, __LINE__, __func__);
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
template aff3ct::module::Radio<double>*      aff3ct::factory::Radio::build<double     >() const;
template aff3ct::module::Radio<float>*       aff3ct::factory::Radio::build<float      >() const;
template aff3ct::module::Radio<short>*       aff3ct::factory::Radio::build<short      >() const;
template aff3ct::module::Radio<signed char>* aff3ct::factory::Radio::build<signed char>() const;
// ==================================================================================== explicit template instantiation

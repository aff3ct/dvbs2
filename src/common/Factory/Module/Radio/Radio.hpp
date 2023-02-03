#ifndef FACTORY_RADIO_HPP_
#define FACTORY_RADIO_HPP_

#include <string>
#include <memory>
#include <aff3ct.hpp>

#include "Module/Radio/Radio.hpp"

namespace aff3ct
{
namespace factory
{
extern const std::string Radio_name;
extern const std::string Radio_prefix;
struct Radio : Factory
{
public:
	// ------------------------------------------------------------------------------------------------- PARAMETERS
	// required parameters

	// optional parameters
	int N                      = 0;
	bool threaded              = false;
	uint64_t fifo_size         = uint64_t(10000000000);
	int n_frames               = 1;
	std::string type           = "USRP";
	std::string usrp_addr      = "192.168.20.2";
	double clk_rate            = 125e6;

	bool rx_enabled            = false;
	double rx_rate             = 0; // if rx_rate is not overriden, rx is disabled
	std::string rx_subdev_spec = "A:0";
	std::string rx_antenna      = "RX2";
	double rx_freq             = 1090e6;
	double rx_gain             = 10;
	std::string rx_filepath    = "";
	std::string tx_filepath    = "";
	bool   rx_no_loop          = false;

	bool tx_enabled            = false;
	double tx_rate             = 0; // if tx_rate is not overriden, tx is disabled
	std::string tx_subdev_spec = "A:0";
	std::string tx_antenna     = "TX/RX";
	double tx_freq             = 1090e6;
	double tx_gain             = 10;

	// deduced parameters

	// ---------------------------------------------------------------------------------------------------- METHODS
	explicit Radio(const std::string &p = Radio_prefix);
	virtual ~Radio() = default;
	Radio* clone() const;

	// parameters construction
	virtual void get_description(cli::Argument_map_info &args) const;
	virtual void store          (const cli::Argument_map_value &vals);
	virtual void get_headers    (std::map<std::string,header_list>& headers, const bool full = true) const;


	template <typename R = float>
	module::Radio<R>* build() const;
};
}
}

#endif /* FACTORY_RADIO_HPP_ */


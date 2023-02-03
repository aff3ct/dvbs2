#ifndef RADIO_USER_BINARY_HPP
#define RADIO_USER_BINARY_HPP

#include <thread>
#include <fstream>

#include "Module/Radio/Radio.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = double>
class Radio_user_binary : public Radio<R>
{
private:
	std::ifstream input_file;
	std::ofstream output_file;
	bool auto_reset;
	bool done;

public:
	Radio_user_binary(const int N,
	                  const std::string &input_filename,
	                  const std::string &output_filename,
	                  const bool auto_reset,
	                  const int n_frames = 1);

	Radio_user_binary(const int N,
	                  const std::string &input_filename,
	                  const std::string &output_filename,
	                  const int n_frames = 1);
	virtual ~Radio_user_binary() = default;

	bool is_done() const;
	void reset();

protected:
	void _send   (const R *X_N1, const int frame_id);
	void _receive(      R *Y_N1, const int frame_id);
};
}
}

#endif /* RADIO_USER_BINARY_HPP */

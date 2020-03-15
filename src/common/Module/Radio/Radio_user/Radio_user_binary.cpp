#include <typeinfo>
#include <fstream>
#include <string>

#include "Module/Radio/Radio_user/Radio_user_binary.hpp"

using namespace aff3ct;
using namespace aff3ct::module;

template <typename R>
Radio_user_binary<R>
::Radio_user_binary(const int N,
                    const std::string &input_filename,
                    const std::string &output_filename,
                    const int n_frames)
: Radio<R>(N, n_frames)
{
	const std::string name = "Radio_user_binary";
	this->set_name(name);

	if (!input_filename.empty())
	{
		input_file.open(input_filename.c_str(), std::ios::in | std::ios::binary);
		if (input_file.fail())
		{
			std::stringstream message;
			message << "'input_filename' file name is invalid: failbit is set.";
			throw tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
		}
	}

	if (!output_filename.empty())
	{
		output_file.open(output_filename.c_str(), std::ios::out | std::ios::binary);
		if (output_file.fail())
		{
			std::stringstream message;
			message << "'output_filename' file name is invalid: failbit is set.";
			throw tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
		}
	}
}

template <typename R>
void Radio_user_binary<R>
::_send(const R *X_N1, const int frame_id)
{
	if (!output_file.is_open())
	{
		std::stringstream message;
		message << "'input_file' is not open.";
		throw tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
	}

	output_file.write(reinterpret_cast<const char*>(X_N1), 2 * this->N * sizeof(R));

	if (output_file.fail())
		throw tools::runtime_error(__FILE__, __LINE__, __func__, "Unknown error during file reading.");
}

template <typename R>
void Radio_user_binary<R>
::_receive(R *Y_N1, const int frame_id)
{
	if (!input_file.is_open())
	{
		std::stringstream message;
		message << "'input_file' is not open.";
		throw tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
	}

	input_file.read(reinterpret_cast<char*>(Y_N1), 2 * this->N * sizeof(R));

	if (input_file.fail())
	{
		if (input_file.eof())
		{
			// throw tools::runtime_error(__FILE__, __LINE__, __func__, "Radio USER_BIN reached EOF.");
			input_file.clear();
			input_file.seekg(0, std::ios::beg);
		}

		if (input_file.fail())
			throw tools::runtime_error(__FILE__, __LINE__, __func__, "Unknown error during file reading.");
	}
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Radio_user_binary<double>;
template class aff3ct::module::Radio_user_binary<float>;
template class aff3ct::module::Radio_user_binary<int16_t>;
template class aff3ct::module::Radio_user_binary<int8_t>;
// ==================================================================================== explicit template instantiation

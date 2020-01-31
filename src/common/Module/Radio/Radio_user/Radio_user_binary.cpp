#include <typeinfo>
#include <fstream>
#include <string>

#include "Module/Radio/Radio_user/Radio_user_binary.hpp"

using namespace aff3ct;
using namespace aff3ct::module;

template <typename B>
std::thread::id aff3ct::module::Radio_user_binary<B>::master_thread_id = std::this_thread::get_id();

template <typename R>
Radio_user_binary<R>
::Radio_user_binary(const int N, const std::string filename, const int n_frames)
: Radio<R>(N, n_frames), source_file(filename.c_str(), std::ios::in | std::ios::binary)
{
	const std::string name = "Radio_user_binary";
	this->set_name(name);

	if (this->master_thread_id != std::this_thread::get_id())
	{
		std::stringstream message;
		message << name << " is not thread safe.";
		throw tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (source_file.fail())
	{
		std::stringstream message;
		message << "'filename' file name is not valid: sink file failbit is set.";
		throw tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (source_file.is_open())
	{
		unsigned n_fra = 0;
		int fra_size = 0;

		source_file.read((char*)&n_fra,    sizeof(n_fra));
		source_file.read((char*)&fra_size, sizeof(fra_size));
	}
}

template <typename R>
void Radio_user_binary<R>
::_send(const R *X_N1, const int frame_id)
{
	std::stringstream message;
	message << "Send task is undefined for USER BIN radio type.";
	throw tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
}

template <typename R>
void Radio_user_binary<R>
::_receive(R *Y_N1, const int frame_id)
{
	source_file.read(reinterpret_cast<char*>(Y_N1), 2 * this->N * sizeof(R));

	if (source_file.fail())
	{
		if (source_file.eof())
		{
			// throw tools::runtime_error(__FILE__, __LINE__, __func__, "Radio USER_BIN reached EOF.");
			source_file.clear();
			source_file.seekg(0, std::ios::beg);
		}

		if (source_file.fail())
			throw tools::runtime_error(__FILE__, __LINE__, __func__, "Unknown error during file reading.");
	}
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Radio_user_binary<double>;
template class aff3ct::module::Radio_user_binary<float>;
template class aff3ct::module::Radio_user_binary<int16_t>;
template class aff3ct::module::Radio_user_binary<int8_t>;
// ==================================================================================== explicit template instantiation

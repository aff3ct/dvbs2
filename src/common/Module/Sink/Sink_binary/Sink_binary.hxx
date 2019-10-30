/*!
 * \file
 * \brief Send data to a binary file.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */

#ifndef SINK_BINARY_HXX_
#define SINK_BINARY_HXX_

#include <fstream>
#include <sstream>

#include "Tools/Algo/Bit_packer.hpp"
#include "Tools/Exception/exception.hpp"
#include "Module/Source/User/Source_user_binary.hpp"

#include "Module/Sink/Sink_binary/Sink_binary.hpp"

using namespace aff3ct;
using namespace aff3ct::module;

template <typename B>
Sink_binary<B>
::Sink_binary(const int N, const std::string filename, const int n_frames)
: Sink<B>(N, n_frames), sink_file(filename.c_str(), std::ios::out | std::ios::binary)
{
	const std::string name = "Sink_binary";
	this->set_name(name);

	if (this->N < 8)
		throw runtime_error(__FILE__, __LINE__, __func__, "Sink_binary does not manage N < 8");

	if (sink_file.fail())
	{
		std::stringstream message;
		message << "'filename' file name is not valid: sink file failbit is set.";
		throw runtime_error(__FILE__, __LINE__, __func__, message.str());
	}
}

template <typename B>
void Sink_binary<B>
::_send(const B *X_N1, const int frame_id)
{
	static int n_left = 0;                              // number of bits have not been left by last call
	int n_completing  = (CHAR_BIT - n_left) % CHAR_BIT; // number of bits that are needed to complete one byte
	B    reconstructed_buffer[CHAR_BIT];                // to store reconstructed byte (n_left & n_completing)
	char reconstructed_byte;                            // to store reconstructed byte (n_left & n_completing)

	if (sink_file.fail())
	{
		std::stringstream message;
		message << "'filename' file name is not valid: sink file failbit is set.";
		throw runtime_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (n_left != 0)
	{

		for (auto i = 0; i < n_completing; i++) // completing byte with n_completing first bits of X_N1
			reconstructed_buffer[i] = X_N1[n_left + i];
		tools::Bit_packer::pack(reconstructed_buffer, &reconstructed_byte, CHAR_BIT);
		sink_file.write(&reconstructed_byte, 1);
	}

	int main_chunk_size = (this->N - n_completing) / CHAR_BIT; // en octet
	n_left              = (this->N - n_completing) % CHAR_BIT;

	std::vector<char> chunk(main_chunk_size);

	tools::Bit_packer::pack(X_N1 + n_completing, chunk.data(), main_chunk_size * CHAR_BIT);
	sink_file.write(chunk.data(), main_chunk_size);
	sink_file.flush();
	n_left = 0;
	for (auto i = n_completing + main_chunk_size * CHAR_BIT; i < this->N; i++ )
		reconstructed_buffer[n_left++] = X_N1[i];
}

#endif /* SINK_BINARY_HXX_ */

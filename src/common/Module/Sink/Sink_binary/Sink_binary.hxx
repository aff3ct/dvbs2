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
		throw runtime_error(__FILE__, __LINE__, __func__, "Sink_binary does not manage N < 9");

	if (sink_file.fail())
	{
		std::stringstream message;
		message << "'filename' file name is not valid: sink file failbit is set.";
		throw runtime_error(__FILE__, __LINE__, __func__, message.str());
	}
}

template <typename B>
Sink_binary<B>
::~Sink_binary()
{

}

template <typename B>
void Sink_binary<B>
::_send(const B *X_N1, const int frame_id)
{
	B buffer[CHAR_BIT];
	static int n_rest       = 0;
	       int n_consumed = (CHAR_BIT - n_rest) % CHAR_BIT;

	if (n_rest != 0)
	{
		for (auto i = 0; i < n_consumed; i++)
			buffer[i] = X_N1[n_rest + i];
		char solo_byte;
		tools::Bit_packer::pack(buffer, &solo_byte, CHAR_BIT);
		sink_file.write(&solo_byte, 1);
	}

	int main_chunk_size = (this->N - n_consumed) / CHAR_BIT; // en octet
	n_rest              = (this->N - n_consumed) % CHAR_BIT;

	char* chunk = new char[main_chunk_size];

	tools::Bit_packer::pack(X_N1 + n_consumed, chunk, main_chunk_size * CHAR_BIT);
	sink_file.write(chunk, main_chunk_size);
	sink_file.flush();
	n_rest = 0;
	for (auto i = n_consumed + main_chunk_size * CHAR_BIT; i < this->N; i++ )
		buffer[n_rest++] = X_N1[i];

	delete[] chunk;
}

#endif /* SINK_BINARY_HXX_ */

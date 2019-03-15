/*!
 * \file
 * \brief Pushes data to a file.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef SINK_HPP
#define SINK_HPP

#include <vector>
#include <fstream>
#include <string>

/*!
 * \class Sink
 *
 * \brief Pushes data to a file.
 *
 * \tparam D: type of data.
 */
class Sink
{
private:
	const std::string input_filename;
	const std::string output_filename;

	std::ifstream input_file;
	std::ofstream output_file;

	std::vector<std::vector<int>> source;
	int src_counter;

	void pull_vector(int* vec, int N);
	void push_vector(int* vec, int N);

public:
	std::string destination_chain_name;

	Sink(std::string input_filename, std::string output_filename);
	~Sink();

	void push_vector(std::vector<int   > vec, bool iscomplex);
	void push_vector(std::vector<float > vec, bool iscomplex);
	void push_vector(std::vector<double> vec, bool iscomplex);

	void pull_vector(std::vector<int   > &vec);
	void pull_vector(std::vector<float > &vec);
	void pull_vector(std::vector<double> &vec);
};

#endif /* SINK_HPP */

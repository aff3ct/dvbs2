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
#include <iostream>

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

	const std::string filename;
	std::ofstream output_file;

public:
	
	Sink(std::string filename)
	: filename(filename)
	{
		output_file.open (filename);
	}

	void push_vector(std::vector<int  > vec)
	{
		for( int i = 0; i < vec.size(); i++)		
			output_file << vec[i] << " ";
		
		output_file << std::endl;
	};

};
//#include "Sink.cpp"

#endif /* SINK_HPP */

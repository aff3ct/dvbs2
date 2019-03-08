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
	
	void push_vector(int* vec, int N);

public:
	Sink(std::string filename);
	~Sink();

	void push_vector(std::vector<int  > vec, bool iscomplex);
	void push_vector(std::vector<float  > vec, bool iscomplex);
	void push_vector(std::vector<double  > vec, bool iscomplex);
};
//#include "Sink.cpp"

void Sink
::push_vector(int* vec, int N)
{
	for( int i = 0; i < N; i++)		
		output_file << vec[i] << " ";
	
	output_file << std::endl;
};

Sink
::Sink(std::string filename)
: filename(filename), output_file(std::ofstream())
{
	output_file.open (filename);
}


Sink
::~Sink()
{
	output_file.close();
}

void Sink::
push_vector(std::vector<int  > vec, bool is_complex)
{
	if (is_complex)
		output_file << "complex" << std::endl;	
	else
		output_file << "real" << std::endl;
	output_file << "int" << std::endl;
	this->push_vector(vec.data(), vec.size());
	output_file << std::endl;
};

void Sink::
push_vector(std::vector<float  > vec, bool is_complex)
{
	int* int_data_ptr = reinterpret_cast<int *>(vec.data());
	int  int_data_sz  = vec.size()*sizeof(float)/sizeof(int);
	if (is_complex)
		output_file << "complex" << std::endl;	
	else
		output_file << "real" << std::endl;
	output_file << "float" << std::endl;

	this->push_vector(int_data_ptr, int_data_sz);
	
	output_file << std::endl;
};

void Sink::
push_vector(std::vector<double  > vec, bool is_complex)
{
	int* int_data_ptr = reinterpret_cast<int *>(vec.data());
	int  int_data_sz  = vec.size()*sizeof(double)/sizeof(int);
	if (is_complex)
		output_file << "complex" << std::endl;	
	else
		output_file << "real" << std::endl;
	output_file << "double" << std::endl;

	this->push_vector(int_data_ptr, int_data_sz);
	
	output_file << std::endl;
};

#endif /* SINK_HPP */

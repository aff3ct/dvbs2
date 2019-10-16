#include <iostream>

#include <aff3ct.hpp>
using namespace aff3ct;

#include "Sink/Sink.hpp"

Sink
::Sink(const std::string input_filename, const std::string output_filename)
: input_filename(input_filename), output_filename(output_filename), source(), src_counter(0), destination_chain_name()
{
	if (input_filename.empty())
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, "'input_filename' should not be empty.");

	if (output_filename.empty())
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, "'input_filename' should not be empty.");

	input_file .open(input_filename);
	output_file.open(output_filename);

	if (input_file.is_open())
	{
		int n_src = 0, src_size = 0;
		std::getline(input_file, destination_chain_name);

		input_file >> n_src;
		input_file >> src_size;

		if (n_src <= 0 || src_size <= 0)
		{
			std::stringstream message;
			message << "'n_src', and 'src_size' have to be greater than 0 ('n_src' = " << n_src
			        << ", 'src_size' = " << src_size << ").";
			throw tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
		}

		this->source.resize(n_src);
		for (auto i = 0; i < n_src; i++)
			this->source[i].resize(src_size);

		for (auto i = 0; i < n_src; i++)
			for (auto j = 0; j < src_size; j++)
			{
				int bit;
				input_file >> bit;

				this->source[i][j] = bit;
			}

		input_file.close();
	}
	else
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, "Can't open '" + input_filename + "' file.");

	src_counter %= (int)source.size();
}

Sink
::~Sink()
{
	input_file.close();
	output_file.close();
}

void Sink
::pull_vector(int *vec, const int N)
{
	if (N != (int)this->source[this->src_counter].size())
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, "Wrong source size.");

	std::copy(this->source[this->src_counter].begin(),
	          this->source[this->src_counter].end  (),
	          vec);

	this->src_counter = (this->src_counter +1) % (int)this->source.size();
}

void Sink
::pull_vector(std::vector<int> &vec)
{
	pull_vector(vec.data(), vec.size());
}

void Sink
::pull_vector(std::vector<float> &vec)
{
	int* int_data_ptr = reinterpret_cast<int *>(vec.data());
	int  int_data_sz  = vec.size()*sizeof(float)/sizeof(int);
	pull_vector(int_data_ptr, int_data_sz);
}

void Sink
::pull_vector(std::vector<double> &vec)
{
	int* int_data_ptr = reinterpret_cast<int *>(vec.data());
	int  int_data_sz  = vec.size()*sizeof(double)/sizeof(int);
	pull_vector(int_data_ptr, int_data_sz);
}

void Sink
::push_vector(int* vec, int N)
{
	for( int i = 0; i < N; i++)
		output_file << vec[i] << " ";

	output_file << std::endl;
};

void Sink
::push_vector(std::vector<int> vec, bool is_complex)
{
	if (is_complex)
		output_file << "complex" << std::endl;
	else
		output_file << "real" << std::endl;
	output_file << "int" << std::endl;
	this->push_vector(vec.data(), vec.size());
	output_file << std::endl;
};

void Sink
::push_vector(std::vector<float> vec, bool is_complex)
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

void Sink
::push_vector(std::vector<double> vec, bool is_complex)
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

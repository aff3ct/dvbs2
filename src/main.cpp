 #include <vector>
 #include <iostream>
 #include <aff3ct.hpp>
 #include "Sink.hpp"
 #include "BB_scrambler.hpp"
 //#include <vector>
 //#include <algorithm>

int main(int argc, char** argv)
{
	using namespace aff3ct;

	const int K_BCH = 14232;
	const std::string mat2aff_file_name = "../matlab/matlab_to_aff3ct.txt";
	const std::string aff2mat_file_name = "../matlab/aff3ct_to_matlab.txt";

	// buffers to store the data
	      std::vector<int  > scrambler_in ( K_BCH );
	const std::vector<int  > lfsr_init    {1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0};

	// create the AFF3CT objects
	module::Source_user<int> 	source_from_matlab	(K_BCH, mat2aff_file_name);//, 1, 1);
	BB_scrambler 				my_scrambler;
	Sink 						sink_to_matlab		(aff2mat_file_name);
	tools::	Frame_trace<> 		tracer				(15, 1, std::cout);

	// retrieve data from Matlab
	source_from_matlab.generate( scrambler_in );
	
	// Base Band scrambling
	my_scrambler.scramble( scrambler_in );

	// Pushes data to Matlab
	sink_to_matlab.push_vector( scrambler_in );

	//tracer.display_bit_vector( scrambler_in );

	return 0;
}

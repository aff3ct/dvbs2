 #include <vector>
 #include <iostream>
 #include <aff3ct.hpp>
 //#include <vector>
 //#include <algorithm>

int main(int argc, char** argv)
{
	using namespace aff3ct;

	const int K_BCH = 14232;
	const std::string mat2aff_file_name = "../matlab/matlab_to_aff3ct.txt";
	std::ofstream aff2mat_file;

	// buffers to store the data
	std::vector<int  > scrambler_in ( K_BCH );
	const std::vector<int  > lfsr_init{1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0};
	std::vector<int  > lfsr(15);
	int feedback;

	// create the AFF3CT objects
	tools::Frame_trace<> tracer(15, 1, std::cout);
	module::Source_user<int> scrambler_in_source(K_BCH, mat2aff_file_name);//, 1, 1);

	scrambler_in_source.generate( scrambler_in );

	aff2mat_file.open ("../matlab/aff3ct_to_matlab.txt");
	
	// init LFSR
	for( int i = 0; i < 14; i++ )
		lfsr[i] = lfsr_init[i];
	

	//tracer.display_bit_vector(scrambler_in);

	for( int i = 0; i < K_BCH; i++)
	{
		
		// step on LFSR
		feedback = (lfsr[14] + lfsr[13]) % 2;
		std::rotate(lfsr.begin(), lfsr.end()-1, lfsr.end());
		lfsr[0] = feedback;
		
		// apply random bit
		
		scrambler_in[i] = (scrambler_in[i] + feedback) % 2;		
		aff2mat_file << scrambler_in[i] << " ";
		if(i<15){
			//tracer.display_bit_vector(lfsr);
			//std::cout << std::endl;
		}
	}

	aff2mat_file.close();
	//std::cout << std::endl;
	//tracer.display_bit_vector(scrambler_in);

	return 0;
}

 #include <vector>
 #include <iostream>
 #include <aff3ct.hpp>
 #include "Sink.hpp"
 #include "BB_scrambler.hpp"

int main(int argc, char** argv)
{
	using namespace aff3ct;

	const std::string mat2aff_file_name = "../build/matlab_to_aff3ct.txt";
	const std::string aff2mat_file_name = "../build/aff3ct_to_matlab.txt";
	Sink sink_to_matlab    (mat2aff_file_name, aff2mat_file_name);

	if (sink_to_matlab.destination_chain_name == "coding")
	{
		const int K_BCH = 14232;
		const int N_BCH = 14400;

		const std::vector<int  > BCH_gen_poly{1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1};
		// buffers to store the data
		std::vector<int  > scrambler_in(K_BCH);

		std::vector<int  > bch_enc_in(K_BCH);
		std::vector<int  > bch_encoded(N_BCH);

		std::vector<int  > parity(N_BCH-K_BCH);
		std::vector<int  > msg(K_BCH);

		// Tracer
		tools::Frame_trace<>     tracer            (15, 10, std::cout);
		
		// Create the AFF3CT objects
		//module::Source_user<int > source_from_matlab(K_BCH, mat2aff_file_name);//, 1, 1);
		BB_scrambler             my_scrambler;
		tools::BCH_polynomial_generator<int  > poly_gen(16383, 12);
		poly_gen.set_g(BCH_gen_poly);
		module::Encoder_BCH<int > BCH_encoder(K_BCH, N_BCH, poly_gen, 1);
		
		// retrieve data from Matlab
		sink_to_matlab.pull_vector( scrambler_in );
		
		// Base Band scrambling
		my_scrambler.scramble( scrambler_in );

		// reverse message for Matlab compliance
		std:reverse(scrambler_in.begin(), scrambler_in.end()); 

		// BCH Encoding
		BCH_encoder.encode(scrambler_in, bch_encoded);

		// revert parity and msg for Matlab compliance
		parity.assign(bch_encoded.begin(), bch_encoded.begin()+(N_BCH-K_BCH)); // retrieve parity
		std::reverse(parity.begin(), parity.end()); // revert parity bits
		msg.assign(bch_encoded.begin()+(N_BCH-K_BCH), bch_encoded.end()); // retrieve message
		std::reverse(msg.begin(), msg.end()); // revert msg bits

		// swap parity and msg for Matlab compliance
		bch_encoded.insert(bch_encoded.begin(), msg.begin(), msg.end()); 
		bch_encoded.insert(bch_encoded.begin()+K_BCH, parity.begin(), parity.end());

		bch_encoded.erase(bch_encoded.begin()+N_BCH, bch_encoded.end());
		
		// Pushes data to Matlab
		sink_to_matlab.push_vector( bch_encoded , false);
	}
	else
	{
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, "Invalid destination name.");
	}
	
	return 0;
}
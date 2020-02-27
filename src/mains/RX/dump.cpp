#include <vector>
#include <numeric>
#include <string>
#include <iostream>

#include <aff3ct.hpp>

#include "Factory/DVBS2O/DVBS2O.hpp"

using namespace aff3ct;
using namespace aff3ct::module;

// global parameters
const std::string extension = "bin";

// aliases
template<class T> using uptr = std::unique_ptr<T>;

int main(int argc, char** argv)
{
	// get the parameter to configure the tools and modules
	const auto params = factory::DVBS2O(argc, argv);

	// construct modules
	uptr<Radio<>> radio(factory::DVBS2O::build_radio<>(params));

	tools::Dumper dumper;
	dumper.register_data(static_cast<R*>((*radio)[rad::sck::receive::Y_N1].get_dataptr()),
	                     2 * params.p_rad.N,
	                     0,
	                     extension,
	                     true,
	                     params.n_frames);

	std::cout << rang::tag::info << "The samples recording is running, press 'ctrl+c' to stop..." << std::endl;

	uint64_t bytes = 0;
	const unsigned n_err = 1;
	tools::Terminal::init();
	while (!tools::Terminal::is_interrupt())
	{
		(*radio)[rad::tsk::receive].exec();
		for (auto f = 0; f < params.n_frames; f++)
			dumper.add(n_err, f);
		bytes += 2 * params.p_rad.N * params.n_frames * sizeof(R);
		std::cout << "Samples size: " << (bytes / (1024 * 1024)) << " MB" << "\r";
	}
	std::cout << "Samples size: " << (bytes / (1024 * 1024)) << " MB       " << std::endl;
	std::cout << rang::tag::info << "The samples are being written in the '"
	          << params.dump_filename << "." << extension << "' file... ";
	std::cout.flush();

	dumper.dump(params.dump_filename);

	std::cout << "Done." << std::endl;

	return EXIT_SUCCESS;
}
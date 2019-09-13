#include <aff3ct.hpp>

#include "Factory_DVBS2O/Factory_DVBS2O.hpp"

using namespace aff3ct;

int main(int argc, char** argv)
{
	// get the parameter to configure the tools and modules
	const auto params = Params_DVBS2O(argc, argv);


	std::vector<float> samples_vec(2 * 16000, 0.0f);


	// the list of the allocated modules for the simulation
	std::vector<const module::Module*> modules;

	std::unique_ptr<module::Radio<>> radio(Factory_DVBS2O::build_radio<>(params));

	modules = { radio.get()};
	
	// configuration of the module tasks
	for (auto& m : modules)
		for (auto& ta : m->tasks)
		{
			ta->set_autoalloc  (true        ); // enable the automatic allocation of the data in the tasks
			ta->set_autoexec   (false       ); // disable the auto execution mode of the tasks
			ta->set_debug      (false       ); // disable the debug mode
			ta->set_debug_limit(-1          ); // display only the 16 first bits if the debug mode is enabled
			ta->set_debug_precision(8          );
			ta->set_stats      (params.stats); // enable the statistics

			// enable the fast mode (= disable the useless verifs in the tasks) if there is no debug and stats modes
			ta->set_fast(!ta->is_debug() && !ta->is_stats());
		}

	
	using namespace module;

	for (int i = 0 ; i < 3; i++)
	{
		(*radio)[rad::tsk::receive].exec();

		std::copy( (float*)((*radio)[rad::sck::receive::Y_N1].get_dataptr()),
		          ((float*)((*radio)[rad::sck::receive::Y_N1].get_dataptr())) + (2 * 16000),
		          samples_vec.data());
	}	

	// write samples_vec to file

	return EXIT_SUCCESS;
}

#include <vector>
#include <numeric>
#include <string>
#include <iostream>

#include <aff3ct.hpp>

#include "Factory/DVBS2O/DVBS2O.hpp"

using namespace aff3ct;

int main(int argc, char** argv)
{
	// get the parameter to configure the tools and modules
	const auto params = factory::DVBS2O(argc, argv);

	std::vector<std::unique_ptr<tools ::Reporter>>              reporters;
	            std::unique_ptr<tools ::Terminal>               terminal;
	                            tools ::Sigma<>                 noise(1.f,1.f,1.f);

	// the list of the allocated modules for the simulation
	std::vector<const module::Module*> modules;

	tools::Dumper dumper;

	// construct modules
	std::unique_ptr<module::Source<>> source       (factory::DVBS2O::build_source                   <>(params                 ));
	std::unique_ptr<module::Radio<> > radio        (factory::DVBS2O::build_radio                    <>(params                 ));

	// allocate reporters to display results in the terminal
	reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_noise     <>(noise   ))); // report the noise values (Es/N0 and Eb/N0)

	// allocate a terminal that will display the collected data from the reporters
	terminal = std::unique_ptr<tools::Terminal>(new tools::Terminal_std(reporters));

	// display the legend in the terminal
	terminal->legend();

	// fulfill the list of modules
	modules = {source.get(), radio.get()};

	// configuration of the module tasks
	for (auto& m : modules)
		for (auto& ta : m->tasks)
		{
			ta->set_autoalloc      (true        ); // enable the automatic allocation of the data in the tasks
			ta->set_autoexec       (false       ); // disable the auto execution mode of the tasks
			ta->set_debug          (params.debug); // disable the debug mode
			ta->set_debug_limit    (-1          ); // display only the 16 first bits if the debug mode is enabled
			ta->set_debug_precision(8           );
			ta->set_stats          (params.stats); // enable the statistics
			ta->set_fast           (false       ); // disable the fast mode
		}

	using namespace module;

	dumper.register_data(static_cast<R*>((*radio)[rad::sck::receive::Y_N1].get_dataptr()), params.p_rad.N, 0, std::string("bin"), true);

	for (auto i = 0; !terminal->is_interrupt() ; i++)
	{
		(*radio        )[rad::tsk::receive     ].exec();
		// std::cout << "i: " << i << std::endl;
		dumper.add(i);
	}

	dumper.dump(std::string("radio"));

	terminal->final_report();
	terminal->final_report(std::cerr);

	std::cout << "#" << std::endl;
	std::cout << "# End of the simulation" << std::endl;

	return EXIT_SUCCESS;
}
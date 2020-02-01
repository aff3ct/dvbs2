#ifdef DVBS2O_LINK_HWLOC

#ifndef THREAD_PINNING_HPP
#define THREAD_PINNING_HPP

#include <vector>
#include <mutex>
#include <hwloc.h>

namespace aff3ct
{
namespace tools
{
class Thread_pinning
{
protected:
	static hwloc_topology_t topology;
	static int topodepth;
	static std::vector<hwloc_uint64_t> pinned_threads;
	static bool is_init;
	static std::mutex mtx;

	static hwloc_obj_t cur_core_obj;

public:
	static void init();
	static void destroy();

	static hwloc_topology_t& get_topology();

	static int get_topodepth();

	static void pin();
	static void unpin();

	static void example1();
	static void example2();
	static void example3();
	static void example4();
	static void example5();
	static void example6();
};
}
}

#endif /* THREAD_PINNING_HPP */

#endif /* DVBS2O_LINK_HWLOC */

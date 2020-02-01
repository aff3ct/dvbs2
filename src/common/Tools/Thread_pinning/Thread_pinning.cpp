#ifdef DVBS2O_LINK_HWLOC

#include <functional>
#include <iostream>
#include <sstream>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "Tools/Exception/exception.hpp"
#include "Tools/Thread_pinning/Thread_pinning.hpp"

using namespace aff3ct;
using namespace aff3ct::tools;

hwloc_topology_t aff3ct::tools::Thread_pinning::topology;
int aff3ct::tools::Thread_pinning::topodepth = 0;
std::vector<hwloc_uint64_t> aff3ct::tools::Thread_pinning::pinned_threads;
bool aff3ct::tools::Thread_pinning::is_init = false;
std::mutex aff3ct::tools::Thread_pinning::mtx;

hwloc_obj_t aff3ct::tools::Thread_pinning::cur_core_obj;

void Thread_pinning
::init()
{
	if (!Thread_pinning::is_init)
	{
		Thread_pinning::mtx.lock();
		if (!Thread_pinning::is_init)
		{
			Thread_pinning::is_init = true;

			/* Allocate and initialize topology object. */
			hwloc_topology_init(&Thread_pinning::topology);

			/* ... Optionally, put detection configuration here to ignore
			 some objects types, define a synthetic topology, etc....
			 The default is to detect all the objects of the machine that
			 the caller is allowed to access.  See Configure Topology
			 Detection. */

			/* Perform the topology detection. */
			hwloc_topology_load(Thread_pinning::topology);

			/* Optionally, get some additional topology information
			 in case we need the topology depth later. */
			Thread_pinning::topodepth = hwloc_topology_get_depth(Thread_pinning::topology);

			int core_depth = hwloc_get_type_or_below_depth(Thread_pinning::get_topology(), HWLOC_OBJ_CORE);
			/* Get last core. */
			int depth_core = hwloc_get_type_or_below_depth(Thread_pinning::get_topology(), HWLOC_OBJ_CORE);
			int core_id = hwloc_get_nbobjs_by_depth(Thread_pinning::get_topology(), depth_core) -1;
			Thread_pinning::cur_core_obj = hwloc_get_obj_by_depth(Thread_pinning::get_topology(), core_depth, core_id);
		}
		Thread_pinning::mtx.unlock();
	}
}

void Thread_pinning
::destroy()
{
	if (Thread_pinning::is_init)
	{
		Thread_pinning::mtx.lock();
		if (Thread_pinning::is_init)
		{
			/* Destroy topology object. */
			hwloc_topology_destroy(Thread_pinning::topology);

			Thread_pinning::is_init = false;
			Thread_pinning::topodepth = 0;
		}
		Thread_pinning::mtx.unlock();
	}
}

hwloc_topology_t& Thread_pinning
::get_topology()
{
	return Thread_pinning::topology;
}

int Thread_pinning
::get_topodepth()
{
	return Thread_pinning::topodepth;
}

void Thread_pinning
::pin()
{
	Thread_pinning::mtx.lock();

	if (Thread_pinning::cur_core_obj)
	{
		/* Get the first PU of the core */
		if (Thread_pinning::cur_core_obj->arity == 0)
		{
			std::stringstream message;
			message << "Unsupported architecture, a core should have at least one PU.";
			throw tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
		}

		hwloc_obj_t cur_core_pu0_obj = Thread_pinning::cur_core_obj->children[0];

		std::cerr << "Thread pinning info -- logical index (hwloc): " << cur_core_pu0_obj->logical_index
		          << " -- OS index: " << cur_core_pu0_obj->os_index << std::endl;

		/* Get a copy of its cpuset that we may modify. */
		hwloc_cpuset_t cpuset = hwloc_bitmap_dup(cur_core_pu0_obj->cpuset);
		/* Get only one logical processor (in case the core is
		SMT/hyper-threaded). */
		hwloc_bitmap_singlify(cpuset);
		/* And try to bind ourself there. */
		if (hwloc_set_cpubind(Thread_pinning::get_topology(), cpuset, HWLOC_CPUBIND_THREAD))
		{
			char *str;
			int error = errno;
			hwloc_bitmap_asprintf(&str, cur_core_pu0_obj->cpuset);
			printf("Couldn't bind to cpuset %s: %s\n", str, strerror(error));
			free(str);
		}
		/* Free our cpuset copy */
		hwloc_bitmap_free(cpuset);
	}

	/* Get next core. */
	Thread_pinning::cur_core_obj = Thread_pinning::cur_core_obj->prev_cousin;

	if (Thread_pinning::cur_core_obj == nullptr)
	{
		std::stringstream message;
		message << "There is no more available cores.";
		throw tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
	}

	Thread_pinning::mtx.unlock();
}

void Thread_pinning
::unpin()
{
	Thread_pinning::mtx.lock();
	Thread_pinning::mtx.unlock();
}

void Thread_pinning
::example1()
{
	/*****************************************************************
	 * First example:
	 * Walk the topology with an array style, from level 0 (always
	 * the system level) to the lowest level (always the proc level).
	 *****************************************************************/

	char string[128];
	for (int depth = 0; depth < Thread_pinning::get_topodepth(); depth++)
	{
		printf("*** Objects at level %d\n", depth);
		for (unsigned i = 0; i < hwloc_get_nbobjs_by_depth(Thread_pinning::get_topology(), depth); i++)
		{
			hwloc_obj_type_snprintf(string, sizeof(string),
				hwloc_get_obj_by_depth(Thread_pinning::get_topology(), depth, i), 0);
			printf("Index %u: %s\n", i, string);
		}
	}
}

void Thread_pinning
::example2()
{
	/*****************************************************************
	 * Second example:
	 * Walk the topology with a tree style.
	 *****************************************************************/

	std::function<void(hwloc_topology_t, hwloc_obj_t, int)> print_children =
		[&print_children](hwloc_topology_t topology, hwloc_obj_t obj, int depth)
		{
			char type[32], attr[1024];
			unsigned i;
			hwloc_obj_type_snprintf(type, sizeof(type), obj, 0);
			printf("%*s%s", 2*depth, "", type);
			if (obj->os_index != (unsigned) -1)
				printf("#%u", obj->os_index);
			hwloc_obj_attr_snprintf(attr, sizeof(attr), obj, " ", 0);
			if (*attr)
				printf("(%s)", attr);

			// if (obj->type == HWLOC_OBJ_CORE)
			// {
			//   auto arity = obj->arity;
			//   printf(" CORE %d ", arity);

			//   printf("logical index obj = %d \n", obj->logical_index);
			//   auto obj2 = obj->next_cousin;
			//   printf("logical index obj2 = %d \n", obj2->logical_index);

			// }

			printf("\n");
			for (i = 0; i < obj->arity; i++) {
				print_children(topology, obj->children[i], depth + 1);
			}
		};

	printf("*** Printing overall tree\n");
	print_children(Thread_pinning::get_topology(), hwloc_get_root_obj(Thread_pinning::get_topology()), 0);
}

void Thread_pinning
::example3()
{
	/*****************************************************************
	 * Third example:
	 * Print the number of packages.
	 *****************************************************************/

	int depth = hwloc_get_type_depth(Thread_pinning::get_topology(), HWLOC_OBJ_PACKAGE);
	if (depth == HWLOC_TYPE_DEPTH_UNKNOWN)
	{
		printf("*** The number of packages is unknown\n");
	}
	else
	{
		printf("*** %u package(s)\n",
		hwloc_get_nbobjs_by_depth(Thread_pinning::get_topology(), depth));
	}
}

void Thread_pinning
::example4()
{
	/*****************************************************************
	 * Fourth example:
	 * Compute the amount of cache that the first logical processor
	 * has above it.
	 *****************************************************************/

	// int levels = 0;
	// unsigned long size = 0;
	// for (hwloc_obj_t obj = hwloc_get_obj_by_type(Thread_pinning::get_topology(), HWLOC_OBJ_PU, 0); obj; obj = obj->parent)
	// {
	// 	if (obj->type == HWLOC_OBJ_CACHE)
	// 	{
	// 		levels++;
	// 		size += obj->attr->cache.size;
	// 	}
	// }
	// printf("*** Logical processor 0 has %d caches totaling %luKB\n", levels, size / 1024);
}

void Thread_pinning
::example5()
{
	/*****************************************************************
	 * Fifth example:
	 * Bind to only one thread of the last core of the machine.
	 *
	 * First find out where cores are, or else smaller sets of CPUs if
	 * the OS doesn't have the notion of a "core".
	 *****************************************************************/

	int depth = hwloc_get_type_or_below_depth(Thread_pinning::get_topology(), HWLOC_OBJ_CORE);
	/* Get last core. */
	hwloc_obj_t obj = hwloc_get_obj_by_depth(Thread_pinning::get_topology(), depth,
		hwloc_get_nbobjs_by_depth(Thread_pinning::get_topology(), depth) - 1);
	if (obj)
	{
		/* Get a copy of its cpuset that we may modify. */
		hwloc_cpuset_t cpuset = hwloc_bitmap_dup(obj->cpuset);
		/* Get only one logical processor (in case the core is
		SMT/hyper-threaded). */
		hwloc_bitmap_singlify(cpuset);
		/* And try to bind ourself there. */
		if (hwloc_set_cpubind(Thread_pinning::get_topology(), cpuset, 0))
		{
			char *str;
			int error = errno;
			hwloc_bitmap_asprintf(&str, obj->cpuset);
			printf("Couldn't bind to cpuset %s: %s\n", str, strerror(error));
			free(str);
		}
		/* Free our cpuset copy */
		hwloc_bitmap_free(cpuset);
	}
}

void Thread_pinning
::example6()
{
	/*****************************************************************
	 * Sixth example:
	 * Allocate some memory on the last NUMA node, bind some existing
	 * memory to the last NUMA node.
	 *****************************************************************/

	/* Get last node. */
	int n = hwloc_get_nbobjs_by_type(Thread_pinning::get_topology(), HWLOC_OBJ_NUMANODE);
	if (n)
	{
		void *m;
		int size = 1024*1024;
		hwloc_obj_t obj = hwloc_get_obj_by_type(Thread_pinning::get_topology(), HWLOC_OBJ_NUMANODE, n - 1);
		m = hwloc_alloc_membind_nodeset(Thread_pinning::get_topology(), size, obj->nodeset, HWLOC_MEMBIND_BIND, 0);
		hwloc_free(Thread_pinning::get_topology(), m, size);
		m = malloc(size);
		hwloc_set_area_membind_nodeset(Thread_pinning::get_topology(), m, size, obj->nodeset, HWLOC_MEMBIND_BIND, 0);
		free(m);
	}
}

#endif

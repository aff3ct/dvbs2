# Compile & Install

Get the AFF3CT library:

	$ git submodule update --init --recursive

## Linux/MacOS/MinGW

Generate the Makefile and compile the code:

	$ mkdir build
	$ cd build
	$ cmake .. -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wall -funroll-loops -march=native"
	$ make

## Windows (Visual Studio project)

Create the Visual Studio project and compile the code:

	$ mkdir build
	$ cd build
	$ cmake .. -G"Visual Studio 15 2017 Win64" -DCMAKE_CXX_FLAGS="-D_SCL_SECURE_NO_WARNINGS /EHsc"
	$ devenv /build Release my_project.sln

The source code of this project is in the `src/` dir.
The compiled binary is in `build/bin/dvbs2_optique`.
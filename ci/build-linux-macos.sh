#!/bin/bash
set -x

if [ -z "$CXX" ]
then
	echo "Please define the 'CXX' environment variable."
	exit 1
fi

if [ -z "$BUILD" ]
then
	echo "The 'BUILD' environment variable is not set, default value = 'build_linux_macos'."
	BUILD="build_linux_macos"
fi

if [ -z "$THREADS" ]
then
	echo "The 'THREADS' environment variable is not set, default value = 1."
	THREADS=1
fi

if [[ $CXX == icpc ]]; then
	source /opt/intel/vars-intel.sh
fi

mkdir $BUILD
cd $BUILD
cmake .. -G"Unix Makefiles" $CMAKE_OPT -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="$CFLAGS"
rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
make -j $THREADS
rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi

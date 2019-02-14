#!/bin/bash
set -x

if [ -z "$EXAMPLES" ]
then
	echo "Please define the 'EXAMPLES' environment variable."
	exit 1
fi

if [ -z "$CXX" ]
then
	echo "Please define the 'CXX' environment variable."
	exit 1
fi

if [ -z "$AFF3CT_GIT_VERSION" ]
then
	echo "Please define the 'AFF3CT_GIT_VERSION' environment variable."
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

# Compile the AFF3CT library
cd lib/aff3ct
mkdir $BUILD
cd $BUILD
cmake .. -G"Unix Makefiles" -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="$CFLAGS" -DAFF3CT_COMPILE_EXE="OFF" -DAFF3CT_COMPILE_STATIC_LIB="ON" -DCMAKE_INSTALL_PREFIX="install"
rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
make -j $THREADS
rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
make install > /dev/null
rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
cd ..

is_systemc=NO
for example in ${EXAMPLES[*]}; do
	if [[ $example == systemc ]]; then
		is_systemc=YES
	fi
done
if [[ $is_systemc == YES ]]; then
	# Add the 'FindSystemC.cmake' and 'FindTLM.cmake' files
	mkdir cmake
	mkdir cmake/Modules
	cp $SYSTEMC_HOME/FindSystemC.cmake cmake/Modules/
	cp $SYSTEMC_HOME/FindTLM.cmake cmake/Modules/

	# Compile the AFF3CT library with SystemC
	mkdir ${BUILD}_systemc
	cd ${BUILD}_systemc
	cmake .. -G"Unix Makefiles" -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="$CFLAGS" -DAFF3CT_COMPILE_EXE="OFF" -DAFF3CT_COMPILE_STATIC_LIB="ON" -DAFF3CT_SYSTEMC_MODULE="ON" -DCMAKE_INSTALL_PREFIX="install"
	rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
	make -j $THREADS
	rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
	make install > /dev/null
	rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
	cd ..
fi

# Compile all the projects using AFF3CT
cd ../../examples
for example in ${EXAMPLES[*]}; do
	cd $example
	mkdir cmake-config
	if [[ $example == systemc ]]; then
		cp ../../lib/aff3ct/${BUILD}_systemc/lib/cmake/aff3ct-$AFF3CT_GIT_VERSION/* cmake-config
	else
		cp ../../lib/aff3ct/${BUILD}/lib/cmake/aff3ct-$AFF3CT_GIT_VERSION/* cmake-config
	fi
	mkdir $BUILD
	cd $BUILD
	cmake .. -G"Unix Makefiles" -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="$CFLAGS"
	rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
	make -j $THREADS
	rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
	cd ../..
done
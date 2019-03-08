cmake_minimum_required (VERSION 3.2)
cmake_policy(SET CMP0054 NEW)

project (dvbs2_optique)

# Enable C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify bin path
set (EXECUTABLE_OUTPUT_PATH bin/)

# Create the executable from sources
add_executable(dvbs2_optique ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

# Link with the "Threads library (required to link with AFF3CT after)
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Link with AFF3CT
set(AFF3CT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

add_custom_target (aff3ct-config.cmake
                   COMMAND cp "${CMAKE_CURRENT_SOURCE_DIR}/lib/aff3ct/build/lib/cmake/aff3ct/*" ${AFF3CT_DIR}
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_dependencies(dvbs2_optique aff3ct-config.cmake)

find_package(AFF3CT CONFIG 2.3.1 REQUIRED)
target_link_libraries(dvbs2_optique PRIVATE aff3ct::aff3ct-static-lib)

cmake_minimum_required (VERSION 3.2)
cmake_policy(SET CMP0054 NEW)

project (dvbs2_optique)

# Enable C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify bin path
set (EXECUTABLE_OUTPUT_PATH bin/)

# CMake options

# Generate the source files lists
file(GLOB_RECURSE SRC_FILES_CO     ${CMAKE_CURRENT_SOURCE_DIR}/src/common/*)
file(GLOB_RECURSE SRC_FILES_TX     ${CMAKE_CURRENT_SOURCE_DIR}/src/TX/*)
file(GLOB_RECURSE SRC_FILES_RX     ${CMAKE_CURRENT_SOURCE_DIR}/src/RX/*)
file(GLOB_RECURSE SRC_FILES_TX_RX  ${CMAKE_CURRENT_SOURCE_DIR}/src/TX_RX/*)
file(GLOB_RECURSE SRC_FILES_BRIDGE ${CMAKE_CURRENT_SOURCE_DIR}/src/BRIDGE/*)
file(GLOB_RECURSE SRC_FILES_USRP   ${CMAKE_CURRENT_SOURCE_DIR}/src/USRP/*)

# Create the executables from sources
add_library(dvbs2_optique_common STATIC ${SRC_FILES_CO})
add_executable(dvbs2_optique_tx     ${SRC_FILES_TX})
add_executable(dvbs2_optique_rx     ${SRC_FILES_RX})
add_executable(dvbs2_optique_tx_rx  ${SRC_FILES_TX_RX})
add_executable(dvbs2_optique_bridge ${SRC_FILES_BRIDGE})
add_executable(dvbs2_optique_usrp   ${SRC_FILES_USRP})

# Link with common sources
target_link_libraries(dvbs2_optique_tx     PRIVATE dvbs2_optique_common)
target_link_libraries(dvbs2_optique_rx     PRIVATE dvbs2_optique_common)
target_link_libraries(dvbs2_optique_tx_rx  PRIVATE dvbs2_optique_common)
target_link_libraries(dvbs2_optique_bridge PRIVATE dvbs2_optique_common)
target_link_libraries(dvbs2_optique_usrp   PRIVATE dvbs2_optique_common)
target_include_directories(dvbs2_optique_common PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/common)
target_include_directories(dvbs2_optique_tx     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/common)
target_include_directories(dvbs2_optique_rx     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/common)
target_include_directories(dvbs2_optique_tx_rx  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/common)
target_include_directories(dvbs2_optique_bridge PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/common)
target_include_directories(dvbs2_optique_usrp   PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/common)

# Configure the AFF3CT library before compiling it
option(AFF3CT_COMPILE_EXE        ""                                                   OFF            )
option(AFF3CT_COMPILE_STATIC_LIB ""                                                   ON             )
option(AFF3CT_COMPILE_SHARED_LIB ""                                                   OFF            )
option(AFF3CT_LINK_GSL           ""                                                   OFF            )
option(AFF3CT_LINK_MKL           ""                                                   OFF            )
option(AFF3CT_LINK_UHD           "Link with the UHD library (used in the radio usrp)" ON             )
set   (AFF3CT_PREC               "MULTI"                                              CACHE STRING "") # can be '8', '16', '32', '64' or 'MULTI'

# Compile the AFF3CT library
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/aff3ct/)

# Link with AFF3CT
target_link_libraries(dvbs2_optique_common PRIVATE aff3ct-static-lib)
target_link_libraries(dvbs2_optique_tx     PRIVATE aff3ct-static-lib)
target_link_libraries(dvbs2_optique_rx     PRIVATE aff3ct-static-lib)
target_link_libraries(dvbs2_optique_tx_rx  PRIVATE aff3ct-static-lib)
target_link_libraries(dvbs2_optique_bridge PRIVATE aff3ct-static-lib)
target_link_libraries(dvbs2_optique_usrp   PRIVATE aff3ct-static-lib)

# Link with OpenMP
find_package(OpenMP)
if (OpenMP_FOUND)
    # good way to link with OpenMP in the CMake3 style
    if(${CMAKE_VERSION} VERSION_EQUAL "3.9" OR ${CMAKE_VERSION} VERSION_GREATER "3.9")
        target_link_libraries(dvbs2_optique_tx_rx PRIVATE OpenMP::OpenMP_CXX)
    # old an ugly way to link with OpenMP, may not work with all the comiler
    else()
        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
    endif()
endif(OpenMP_FOUND)

# Link with UHD (& boost)
if(AFF3CT_LINK_UHD)
    target_compile_definitions(dvbs2_optique_common PUBLIC 
                               $<BUILD_INTERFACE:AFF3CT_RADIO_USRP>
                               $<INSTALL_INTERFACE:AFF3CT_RADIO_USRP>)

    target_compile_definitions(dvbs2_optique_common PUBLIC 
                               $<BUILD_INTERFACE:AFF3CT_RADIO_USRP>
                               $<INSTALL_INTERFACE:AFF3CT_RADIO_USRP>)                               

    # Link UHD
    find_package (UHD)
    if    (UHD_FOUND)
        include_directories(${UHD_INCLUDE_DIRS})
        target_link_libraries(dvbs2_optique_common PRIVATE ${UHD_LIBRARIES})
    endif (UHD_FOUND)

    # Link Boost
    find_package(Boost COMPONENTS system filesystem REQUIRED)
    if (Boost_FOUND)
        target_link_libraries(dvbs2_optique_common PRIVATE ${Boost_FILESYSTEM_LIBRARY}
                                                        ${Boost_SYSTEM_LIBRARY})
    endif (Boost_FOUND)
    message(STATUS "AFF3CT - UHD: on")
else()
    message(STATUS "AFF3CT - UHD: off")
endif(AFF3CT_LINK_UHD)


cmake_minimum_required (VERSION 3.5)
cmake_policy(SET CMP0054 NEW)

project (dvbs2)

# Enable C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify bin path
set (EXECUTABLE_OUTPUT_PATH bin/)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# DVBS2 Version
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
string(SUBSTRING ${GIT_SHA1} 0 7 GIT_SHA1)
message(STATUS "DVBS2 - Hash: ${GIT_SHA1}")

# CMake options
option(DVBS2_LINK_UHD "Link with the UHD library (used in the radio USRP)" ON)

# Generate the source files lists
file(GLOB_RECURSE SRC_FILES_CO ${CMAKE_CURRENT_SOURCE_DIR}/src/common/*)
set(SRC_FILES_TX               ${CMAKE_CURRENT_SOURCE_DIR}/src/mains/TX/main.cpp)
set(SRC_FILES_TX_VAR           ${CMAKE_CURRENT_SOURCE_DIR}/src/mains/TX_VAR/main.cpp)
set(SRC_FILES_RX               ${CMAKE_CURRENT_SOURCE_DIR}/src/mains/RX/main.cpp)
set(SRC_FILES_RXB              ${CMAKE_CURRENT_SOURCE_DIR}/src/mains/RX/main_bench.cpp)
set(SRC_FILES_RX_DUMP          ${CMAKE_CURRENT_SOURCE_DIR}/src/mains/RX/dump.cpp)
set(SRC_FILES_TX_RX            ${CMAKE_CURRENT_SOURCE_DIR}/src/mains/TX_RX/main.cpp)
set(SRC_FILES_TX_RX_BB         ${CMAKE_CURRENT_SOURCE_DIR}/src/mains/TX_RX_BB/main.cpp)
set(SRC_FILES_CH               ${CMAKE_CURRENT_SOURCE_DIR}/src/mains/CH/main.cpp)

# Auto generate a header containing the last Git SHA1
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/common/version.cpp.in"
               "${CMAKE_CURRENT_SOURCE_DIR}/src/common/version.cpp" @ONLY)
list(APPEND SRC_FILES_CO "${CMAKE_CURRENT_SOURCE_DIR}/src/common/version.cpp")

# Create the executables from sources
add_library   (dvbs2_common STATIC ${SRC_FILES_CO})
add_executable(dvbs2_tx            ${SRC_FILES_TX})
add_executable(dvbs2_tx_var        ${SRC_FILES_TX_VAR})
add_executable(dvbs2_rx            ${SRC_FILES_RX})
add_executable(dvbs2_rxb           ${SRC_FILES_RXB})
add_executable(dvbs2_rx_dump       ${SRC_FILES_RX_DUMP})
add_executable(dvbs2_tx_rx         ${SRC_FILES_TX_RX})
add_executable(dvbs2_tx_rx_bb      ${SRC_FILES_TX_RX_BB})
add_executable(dvbs2_ch            ${SRC_FILES_CH})

# Link with common sources
target_link_libraries(dvbs2_tx       PRIVATE dvbs2_common)
target_link_libraries(dvbs2_tx_var   PRIVATE dvbs2_common)
target_link_libraries(dvbs2_rx       PRIVATE dvbs2_common)
target_link_libraries(dvbs2_rxb      PRIVATE dvbs2_common)
target_link_libraries(dvbs2_rx_dump  PRIVATE dvbs2_common)
target_link_libraries(dvbs2_tx_rx    PRIVATE dvbs2_common)
target_link_libraries(dvbs2_tx_rx_bb PRIVATE dvbs2_common)
target_link_libraries(dvbs2_ch       PRIVATE dvbs2_common)

target_include_directories(dvbs2_common   PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/common)
target_include_directories(dvbs2_tx       PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/common)
target_include_directories(dvbs2_tx_var   PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/common)
target_include_directories(dvbs2_rx       PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/common)
target_include_directories(dvbs2_rxb      PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/common)
target_include_directories(dvbs2_rx_dump  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/common)
target_include_directories(dvbs2_tx_rx    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/common)
target_include_directories(dvbs2_tx_rx_bb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/common)
target_include_directories(dvbs2_ch       PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/common)

# Configure the AFF3CT library before compiling it
option(AFF3CT_COMPILE_EXE        ""      OFF            )
option(AFF3CT_COMPILE_STATIC_LIB ""      ON             )
option(AFF3CT_COMPILE_SHARED_LIB ""      OFF            )
option(AFF3CT_LINK_GSL           ""      OFF            )
option(AFF3CT_LINK_MKL           ""      OFF            )
set   (AFF3CT_PREC               "MULTI" CACHE STRING "") # can be '8', '16', '32', '64' or 'MULTI'

# Compile the AFF3CT library
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/aff3ct/)

# Link with AFF3CT
target_link_libraries(dvbs2_common   PRIVATE aff3ct-static-lib)
target_link_libraries(dvbs2_tx       PRIVATE aff3ct-static-lib)
target_link_libraries(dvbs2_tx_var   PRIVATE aff3ct-static-lib)
target_link_libraries(dvbs2_rx       PRIVATE aff3ct-static-lib)
target_link_libraries(dvbs2_rxb      PRIVATE aff3ct-static-lib)
target_link_libraries(dvbs2_rx_dump  PRIVATE aff3ct-static-lib)
target_link_libraries(dvbs2_tx_rx    PRIVATE aff3ct-static-lib)
target_link_libraries(dvbs2_tx_rx_bb PRIVATE aff3ct-static-lib)
target_link_libraries(dvbs2_ch       PRIVATE aff3ct-static-lib)

# Link with UHD (& boost)
if(DVBS2_LINK_UHD)
    message(STATUS "DVBS2 - UHD: on")
    target_compile_definitions(dvbs2_common PUBLIC
                               $<BUILD_INTERFACE:DVBS2_LINK_UHD>
                               $<INSTALL_INTERFACE:DVBS2_LINK_UHD>)

    # Link UHD
    find_package (UHD REQUIRED)
    if    (UHD_FOUND)
        include_directories(${UHD_INCLUDE_DIRS})
        target_link_libraries(dvbs2_common PRIVATE ${UHD_LIBRARIES})
    endif (UHD_FOUND)

    # Link Boost
    find_package(Boost QUIET COMPONENTS system filesystem thread REQUIRED)
    if (Boost_FOUND)
        target_link_libraries(dvbs2_common PRIVATE ${Boost_FILESYSTEM_LIBRARY}
                                                   ${Boost_SYSTEM_LIBRARY}
                                                   ${Boost_THREAD_LIBRARY})
    endif (Boost_FOUND)
else()
    message(STATUS "DVBS2 - UHD: off")
endif(DVBS2_LINK_UHD)

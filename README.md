# Compile & Install

Get the AFF3CT library:

```bash
git submodule update --init --recursive
```

## Linux/MacOS/MinGW

Generate the Makefile and compile the code:

```bash
mkdir build
cd build
cmake .. -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wall -funroll-loops -march=native"
make
```

## Windows (Visual Studio project)

Create the Visual Studio project and compile the code:

```bash
mkdir build
cd build
cmake .. -G"Visual Studio 15 2017 Win64" -DCMAKE_CXX_FLAGS="-D_SCL_SECURE_NO_WARNINGS /EHsc"
devenv /build Release dvbs2_optique.sln
```

## Organization of the code

The source code of this project is in the `src/` directory:
- `src/TX`: source code of the transmitter,
- `src/RX`: source code of the receiver,
- `src/TX_RX`: source code of the Monte-Carlo simulation with the transmitter and the receiver,
- `src/USRP`: source code of the chain with USRP modules,
- `src/common`: source code common to the transmitter and the receiver.

The compiled binaries are:
- `build/bin/dvbs2_optique_tx`: the transmitter,
- `build/bin/dvbs2_optique_rx`: the receiver,
- `build/bin/dvbs2_optique_usrp`: the chain with USRP modules,
- `build/bin/dvbs2_optique_tx_rx`: the Monte-Carlo simulation of the transmitter and the receiver.

# Run

To select the number of threads used by openmp, run the binary while setting the `OMP_NUM_THREADS` variable : 
```bash 
OMP_NUM_THREADS=8 ./bin/dvbs2_optique_tx_rx
```

# UHD
Info at [https://kb.ettus.com/USRP_N300/N310/N320/N321_Getting_Started_Guide] for installation.

If no radio is needed and you don't want to link UHD library, add the followin cmake option: `-DAFF3CT_LINK_UHD=OFF`.

Add the following in `~/.bashrc` or `/etc/profile`:
```bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
export UHD_LOG_FILE="./usrp.log"
```

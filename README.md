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

The source code of this project is in the `src/` directory:
- `src/TX`: source code of the transmitter,
- `src/RX`: source code of the receiver,
- `src/TX_RX`: source code of the Monte-Carlo simulation with the transmitter and the receiver,
- `src/common`: source code common to the transmitter and the receiver.

The compiled binaries are:
- `build/bin/dvbs2_optique_tx`: the transmitter,
- `build/bin/dvbs2_optique_rx`: the receiver,
- `build/bin/dvbs2_optique_tx_rx`: the Monte-Carlo simulation of the transmitter and the receiver.

# Run

To select the number of threads used by openmp, run the binary while setting the `OMP_NUM_THREADS` variable : 
```bash 
OMP_NUM_THREADS=8 ./bin/dvbs2_optique_tx_rx
```

# DVBS2 Optique

## Machines installation

- Ubuntu 16.04 Desktop and 18.04 Server have been tested
- Needs Git and CMake:

```bash
sudo apt install git cmake
```

### UHD Installation

UHD is the software library that is needed for controlling the USRPs. Follow the
instructions in "Install Linux" part at
[Ettus N310 Building](https://kb.ettus.com/Building_and_Installing_the_USRP_Open-Source_Toolchain_(UHD_and_GNU_Radio)_on_Linux).

When asked to checkout a specific tag for UHD, use the following:
`v3.14.1.1-rc1`
When asked to checkout a specific tag for GNU radio, use the following:
`3.7.13.4`

To update the USRP's FPGA images to switch between 1Gb / 10Gb / Dual 10Gb
Ethernet, follow the instructions at
[Ettus N310 Getting Started](https://kb.ettus.com/USRP_N300/N310/N320/N321_Getting_Started_Guide),
"Updating the FPGA Image".

Finally, add the following in `~/.bashrc` or `/etc/profile`:

```bash
export UHD_LOG_FILE="./usrp.log"
```

### Ethernet configuration

Connect the USRP on a 10Gb ports. Then configure the IP address and MTU.

- Example file for 16.04 Desktop : `/etc/network/interfaces`

```bash
# interfaces(5) file used by ifup(8) and ifdown(8)
auto lo
iface lo inet loopback

auto eth2
iface eth2 inet static
    address 192.168.20.1/24
    mtu 8000
```

- Example file for 18.04 Server: `/etc/netplan/50-cloud-init.yaml/`


```bash
network:
    ethernets:
        enp0s31f6:
            addresses: [192.168.222.59/24]
            gateway4: 192.168.222.254
            nameservers:
              addresses: [192.168.222.8]
        enp79s0f0:
            addresses: [192.168.20.1/24]
            gateway4: 0.0.0.0
            mtu: 8000
        enp79s0f1:
            addresses: [192.168.10.1/24]
            gateway4: 0.0.0.0
            mtu: 8000
    version: 2
```

### Benchmark

To validate the installation, you may run the following benchmark:

```bash
/usr/local/lib/uhd/examples/benchmark_rate  \
  --args "type=n3xx,addr=192.168.20.2,master_clock_rate=125e6" \
  --duration 60 \
  --channels "0" \
  --rx_rate 31.25e6 \
  --rx_subdev "A:0" \
  --tx_rate 31.25e6 \
  --tx_subdev "A:0"
```

## Compile & Install

Get the AFF3CT library:

```bash
git submodule update --init --recursive
```

### Linux/MacOS/MinGW

Generate the Makefile and compile the code:

```bash
mkdir build
cd build
cmake .. -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wall -funroll-loops -march=native"
make
```

### Windows (Visual Studio project)

Create the Visual Studio project and compile the code:

```bash
mkdir build
cd build
cmake .. -G"Visual Studio 15 2017 Win64" -DCMAKE_CXX_FLAGS="-D_SCL_SECURE_NO_WARNINGS /EHsc"
devenv /build Release dvbs2_optique.sln
```

## Binaries

The source code of this project is in the `src/` directory:
- `src/TX`: source code of the transmitter,
- `src/RX`: source code of the receiver,
- `src/TX_RX`: source code of the Monte-Carlo simulation with the transmitter
and the receiver,
- `src/TX_RX_BB`: source code of the Monte-Carlo simulation without filters and
synchro,
- `src/matlab`: source code of the rx, tx and bridge chain to communicate with
matlab,
- `src/common`: source code common to the transmitter and the receiver.

The compiled binaries are:
- `build/bin/dvbs2_optique_tx`: the transmitter,
- `build/bin/dvbs2_optique_rx`: the receiver,
- `build/bin/dvbs2_optique_tx_rx`: the Monte-Carlo simulation of the transmitter
and the receiver,
- `build/bin/dvbs2_optique_tx_rx_bb`: the Monte-Carlo simulation of the
transmitter and the receiver without filters and synchro,
- `build/bin/dvbs2_optique_matlab_tx`: bridge to matlab,
- `build/bin/dvbs2_optique_matlab_tx`: the transmitter to be launched from
matlab,
- `build/bin/dvbs2_optique_matlab_tx`: the receiver to be launched from matlab.

## Run

### Offline

Some refs with according command line instructions can be found in the `refs/`
directory for  `build/bin/dvbs2_optique_tx_rx` and
`build/bin/dvbs2_optique_tx_rx_bb`.

### With Radio, BER / FER

Here are example command lines for TX and RX, considering `8PSK-S8_9`:

```bash
./bin/dvbs2_optique_tx --rad-tx-rate 1.953125e6 --rad-tx-freq 2360e6 --rad-tx-gain 60 --src-type USER  --sim-stats --mod-cod QPSK-S_8/9
```

```bash
./bin/dvbs2_optique_rx --rad-rx-rate 1.953125e6 --snk-path "dummy.ts" --rad-rx-freq 2360e6 --rad-rx-gain 60 --src-type USER --dec-implem MS --dec-ite 10 --sim-stats --frame-sync-fast --sim-threaded  --mod-cod QPSK-S_8/9
```

### With Radio, Video Streaming

An convenient setup is to use a first computer for TX, a second computer for RX,
and a third for displaying the video.

- Convert a video to TS format (transport stream) using vlc for example
- Stream that video from TX computer:

```bash
./bin/dvbs2_optique_tx --rad-tx-rate 1.953125e6 --rad-tx-freq 2360e6 --rad-tx-gain 60 --src-type USER_BIN --sim-stats --shp-osf 4 --mod-cod QPSK-S_8/9 --src-path airbus.ts
```

- Create a fifo on RX computer `mkfifo stream.fifo`
- Receive the video and write into this fifo

```bash
./bin/dvbs2_optique_rx --rad-rx-rate 1.953125e6 --snk-path "dump.bin" --rad-rx-freq 2360e6 --rad-rx-gain 40 --src-type USER --dec-implem MS --dec-ite 10 --mod-c
od QPSK-S_8/9 --rad-rx-subdev-spec "A:0" --rad-threaded --snk-path stream.fifo
```

- On the third computer, connected by ssh to the RX computer, stream and display
the video via ssh:

```bash
 ssh <rx ip address>  "cat /path/to/fifo/stream.fifo" | cvlc -
```

## OpenMP

To avoid OpenMP linking, add the following cmake option:
`-DDVBS2O_LINK_OPENMP=OFF`.
To select the number of threads used by openmp in `dvbs2_optique_tx_rx_bb`, run
the binary while setting the `OMP_NUM_THREADS` variable:

```bash
OMP_NUM_THREADS=8 ./bin/dvbs2_optique_tx_rx_bb
```

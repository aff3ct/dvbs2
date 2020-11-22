# DVB-S2

## Machines installation

- Ubuntu 16.04 Desktop and 18.04 Server have been tested
- Needs `Git`, `CMake` and `hwloc`:

```bash
sudo apt install git cmake hwloc libhwloc-dev
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

- Example file for 18.04 Server: `/etc/netplan/50-cloud-init.yaml`


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

Generate the Makefile and compile the DVB-S2 project:

```bash
mkdir build
cd build
cmake .. -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wall -funroll-loops -march=native" -DAFF3CT_LINK_HWLOC=ON
make -j20
```

## Binaries

The source code of this project is in the `src/` directory:
- `src/mains/TX`: source code of the transmitter,
- `src/mains/TX_VAR`: source code of the transmitter with variable SNRs,
- `src/mains/RX`: source code of the receiver,
- `src/mains/TX_RX`: source code of the Monte-Carlo simulation with the
transmitter and the receiver,
- `src/mains/TX_RX_BB`: source code of the Monte-Carlo simulation without
filters and synchro,
- `src/mains/MATLAB`: source code of the rx, tx and bridge chain to communicate
with MATLAB,
- `src/common`: source code common to the transmitter and the receiver.

The compiled binaries are:
- `build/bin/dvbs2_tx`: the transmitter,
- `build/bin/dvbs2_tx_var`: the transmitter with variable SNRs,
- `build/bin/dvbs2_rx`: the receiver,
- `build/bin/dvbs2_rx_dump`: dumps the symbols received by the RX in the
`dump.bin` file,
- `build/bin/dvbs2_tx_rx`: the Monte-Carlo simulation of the transmitter
and the receiver,
- `build/bin/dvbs2_tx_rx_bb`: the Monte-Carlo simulation of the
transmitter and the receiver without filters and synchro,
- `build/bin/dvbs2_matlab_bridge`: bridge to MATLAB,
- `build/bin/dvbs2_matlab_tx`: the transmitter to be launched from
MATLAB,
- `build/bin/dvbs2_matlab_rx`: the receiver to be launched from MATLAB.

## Run

### Simulation

Some refs with according command line instructions can be found in the `refs/`
directory for `build/bin/dvbs2_tx_rx` and
`build/bin/dvbs2_tx_rx_bb`.

### Radio

#### BER / FER

Here are example command lines for RX and TX, considering `QPSK-S_8/9`:

```bash
./bin/dvbs2_rx --sim-stats --rad-threaded --rad-rx-subdev-spec "A:0" --rad-rx-rate 30e6 --rad-rx-freq 2360e6 --rad-rx-gain 20 -F 16 --src-type USER --src-path ../conf/src/K_14232.src --mod-cod QPSK-S_8/9 --dec-implem NMS --dec-ite 10 --dec-simd INTER
```

```bash
./bin/dvbs2_tx --sim-stats --rad-threaded --rad-rx-subdev-spec "A:0" --rad-tx-rate 30e6 --rad-tx-freq 2360e6 --rad-tx-gain 30 -F  8 --src-type USER --src-path ../conf/src/K_14232.src --mod-cod QPSK-S_8/9
```

#### Video Streaming

An convenient setup is to use a first computer for TX, a second computer for RX,
and a third for displaying the video.

- Convert a video to TS format (transport stream) using `VLC` for example
- Stream that video from TX computer:

```bash
./bin/dvbs2_tx --sim-stats --rad-threaded --rad-rx-subdev-spec "A:0" --rad-tx-rate 30e6 --rad-tx-freq 2360e6 --rad-tx-gain 30 -F  8 --src-type USER_BIN --src-path /path/to/input/ts/video.ts --mod-cod QPSK-S_8/9
```

- Create a fifo on RX computer `mkfifo output_stream_fifo.ts`
- Receive the video and write into this fifo

```bash
./bin/dvbs2_rx --sim-stats --rad-threaded --rad-rx-subdev-spec "A:0" --rad-rx-rate 30e6 --rad-rx-freq 2360e6 --rad-rx-gain 20 -F 16 --mod-cod QPSK-S_8/9 --dec-implem NMS --dec-ite 10 --dec-simd INTER --snk-path output_stream_fifo.ts
```

- On the third computer, connected by ssh to the RX computer, stream and display
the video via ssh:

```bash
 ssh <rx ip address>  "cat /path/to/fifo/output_stream_fifo.ts" | cvlc -
```

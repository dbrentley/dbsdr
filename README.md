Beginning of a GL based SDR program. Still very much a work in progress.

I've taken code from all over GitHub (and the web), along with my own contributions, to piece this together.

## Installation

```bash
$ sudo apt install cmake libglew-dev libglfw3-dev libhackrf-dev libstb-dev libfftw3-dev
$ git clone https://github.com/dbrentley/dbsdr.git
$ cd dbsdr
$ mkdir build && cd build
$ cmake ..
$ make
```

## Run

```bash
$ ./dbsdr
```
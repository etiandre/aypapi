# aypapi

## Usage
~~~~
Usage: aypapi [OPTION...]

  -s, --sockets=uncore_list  Comma-separated list of sockets / package indexes
                             to meter.
  -t, --sleeptime=time       Time in seconds between two measurments.
  -v, --verbose              Enable debug output.
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.
~~~~

## Installation
You'll need a working installation of PAPI and hwloc. The makefile also requires pkg-config to find these.

Then type:
~~~~
make
~~~~

`make install` installs by default to `/usr/local/`.

## Examples
Meter on sockets 0 and 1 every 10ms and log output into out.tsv
~~~~
aypapi -s 0,1 -t 0.01 > out.tsv 
~~~~

## Troubleshooting
  - You'll likely need to disable hyperthreading to get reliable results.
  - You need root access to use this tool.
  - If aypapi can't start, please check that the events used are availiable using `papi_native_avail`. If not, you may need to activate the msr kernel module and to allow acces to linux perf events by executing (as root) :
  ~~~~
  modprobe msr
  echo -1 >/proc/sys/kernel/perf_event_paranoid
  ~~~~

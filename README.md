# Ay Papi

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

## Compilaton
~~~~
make install
~~~~

## Examples
Meter on sockets 0 and 1 every 10ms and log output into out.tsv
~~~~
aypapi -s 0,1 -t 0.01 > out.tsv 
~~~~

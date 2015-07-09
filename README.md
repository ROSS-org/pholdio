pholdio
========

PHOLD is a well known PDES benchmark.
This version of the model works with the [gonsie/RIO](http://github.com/gonsie/RIO) API to demonstate proper use of the checkpointing functionality.

## Usage

PHOLDIO opperates with the same command line flags as [PHOLD](https://github.com/carothersc/ROSS/tree/master/models/phold). PHOLDIO Implements one additional flag: `--io-store=[0,1]`.

When `--io-store=0`, the PHOLDIO model will attempt to start from a checkpoint named "pholdio_checkpoint". This is the default value for io-store.

When `--io-store=1`, the PHOLDIO model will save a checkpoint with the name "pholdio_checkpoint" after the simulation has completed. 

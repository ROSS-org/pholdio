PHOLD-IO
========

PHOLD is a well known PDES benchmark.
This repository contains two versions:

- pholdio: which implements the [ross-org/RIO](http://github.com/ross-org/RIO) API to demonstrate proper use of the checkpointing functionality
- dpholdio: a version of [phold-delta](https://github.com/ross-org/ROSS-Models/tree/master/phold-delta) which implements the RIO API.
  LPs in this model contain a large state.

## Building

The dpholdio model can be built with or without the RIO library (the `USE_RIO` option in CMake).
The pholdio model requires the RIO library.

## Usage

Both models operate with the same command line flags as [PHOLD](https://github.com/ross-org/ROSS/tree/master/models/phold).
There is one additional flag: `--io-store=[0,1]`.

When `--io-store=0`, the models will attempt to start from a checkpoint named "pholdio_checkpoint". This is the default value for io-store.

When `--io-store=1`, the models will save a checkpoint with the name "pholdio_checkpoint" after the simulation has completed.

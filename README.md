## Overview

fsdyn is a C library for Linux-like operating systems that offers a
number of abstract data types and useful functions.

## Building

fsdyn uses [SCons][] and `pkg-config` for building.

Before building fsdyn for the first time, run
```
git submodule update --init
```

To build fsdyn, run
```
scons [ prefix=<prefix> ]
```
from the top-level fsdyn directory. The optional prefix argument is a
directory, `/usr/local` by default, where the build system installs
fsdyn.

To install fsdyn, run
```
sudo scons [ prefix=<prefix> ] install
```

## Documentation

The header files under `include` contain detailed documentation.

[SCons]: https://scons.org/

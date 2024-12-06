# FIT FRED
## About
This repository contains the FRED MAPI code for ALICE's FIT detector.

## Build & Run
Required - `devtoolset-7`.
```
git submodule update --init
cd core
cmake3 . -DMAPI=1
make
cd ..
bin/FREDServer
```

## Documentation
See the [fit-fred wiki](https://github.com/frun36/fit-fred/wiki).

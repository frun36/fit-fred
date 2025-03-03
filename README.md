# FIT FRED
## About
This repository contains the FRED MAPI code for ALICE's FIT detector.

## Setup
This version has been developed and tested using CentOS 7 with `devtoolset-7`. To enable it run:
```
scl enable devtoolset-7 bash
```
Further CentOS configuration is described in the [fit-fred wiki](https://github.com/frun36/fit-fred/wiki).

Before running FRED, you need to configure it for your specific FEE setup. This involves:
- PM services configuration - add or remove entries to the example (PMA0+PMC0) `MAPPING` section in `sections/pm.section`.
- Configure the DIM and database connections in `config/fred.conf`:
```
DIM = [DIM_DNS_NODE]
NAME = [NAME]
DB_USER = [DB_USERNAME]
DB_PASS = [DB_PASSWORD]
DB_CONN = ( DESCRIPTION = (ADDRESS = (PROTOCOL = TCP) (HOST = [DB_HOSTNAME]) (PORT = [PORT])) (LOAD_BALANCE = ON) (ENABLE = BROKEN) (CONNECT_DATA = (SERVER = DEDICATED) (SERVICE_NAME = [DB_SERVICE_NAME])))
```

## Build & Run
```
git submodule update --init # fetch the FRED core (CERN account required)
cd core
cmake3 . -DMAPI=1
make
cd ..
bin/FREDServer
```

## Documentation
See the [fit-fred wiki](https://github.com/frun36/fit-fred/wiki) for further information.

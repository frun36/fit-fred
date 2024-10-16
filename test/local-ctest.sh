#!/bin/bash
cmake -S .. -B build
cd build
make && ctest --verbose --output-on-failure
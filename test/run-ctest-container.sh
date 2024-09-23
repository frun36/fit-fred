#!/bin/bash
docker build -t fred-server-env -f ../core/Dockerfile ../core
docker build -t fit-fred-env -f ENV.Dockerfile ..
docker build --no-cache -t fred-server -f Dockerfile .. && docker run --rm fred-server ctest --verbose --output-on-failure

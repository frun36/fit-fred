#!/bin/bash
docker build -t cc7-fit -f ../CC7-image/Dockerfile ..
docker build --no-cache -t fred-server -f Dockerfile .. && docker run --rm fred-server ctest3 --verbose --output-on-failure

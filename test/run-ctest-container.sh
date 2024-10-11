#!/bin/bash
docker build -t cc7-fit -f ../CC7-image/Dockerfile ../CC7-image/ --ulimit nofile=1024:262144
docker build --no-cache -t fred-server -f Dockerfile .. && docker run --rm fred-server ctest3 --verbose --output-on-failure

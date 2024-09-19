#!bin/bash
docker build --no-cache -t fred-server .
docker run --rm fred-server ctest
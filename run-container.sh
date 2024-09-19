#!bin/bash
docker build --no-cache -t fred-server .
docker run fred-server ctest
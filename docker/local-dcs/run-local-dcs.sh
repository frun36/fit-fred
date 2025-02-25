#!/bin/bash

# Set DIM_HOST_NODE environment variable
source FRED/build.sh
source AlfIPbus/build.sh
source Mock/build.sh

rm -f $(pwd)/common-storage/output/*-log 

# Prepare Docker network
docker network inspect alfipbus-tester-network >/dev/null 2>&1 || \
docker network create --driver=bridge --subnet=172.25.175.0/24 local-dcs-network

# Run Mock container
docker run -i -d --name local-dcs-mock --rm \
    --mount type=bind,source=$(pwd)/common-storage,target=/common-storage \
    --network local-dcs-network \
    --add-host host.docker.internal:host-gateway \
    --ip 172.25.175.10 alfipbus-tester

docker exec -d -e LD_LIBRARY_PATH="/usr/lib" local-dcs-mock /bin/bash -c \
    "/alf-ipbus-tester/build/bin/mock -c /common-storage/mock-config.toml -f /common-storage/output/mock-log -v"

# Run ALF container
docker run -i -d --name  local-dcs-alf --rm \
    --mount type=bind,source=$(pwd)/common-storage,target=/common-storage \
    --network local-dcs-network \
    --ip 172.25.175.12 \
    --add-host host.docker.internal:host-gateway alf-ipbus

docker exec -d -e LD_LIBRARY_PATH="/usr/lib" -e DIM_HOST_NODE=172.25.175.12 -e DIM_DNS_NODE=host.docker.internal local-dcs-alf /bin/bash -c \
    "AlfIPbus -n ALF -l 172.25.175.10:50001 -f /common-storage/output/alf-log -t 1000 -v"

# Run ALF container
docker run -i -d --name  local-dcs-fred --rm \
    --mount type=bind,source=$(pwd)/common-storage,target=/common-storage \
    --network local-dcs-network \
    --ip 172.25.175.20 \
    --add-host host.docker.internal:host-gateway fit-fred

docker exec -d -e LD_LIBRARY_PATH="/usr/lib" -e DIM_HOST_NODE=172.25.175.20 -e DIM_DNS_NODE=host.docker.internal local-dcs-fred /bin/bash -c \
    "cd .. && ./bin/FREDServer --verbose --log /common-storage/output/fred-log"


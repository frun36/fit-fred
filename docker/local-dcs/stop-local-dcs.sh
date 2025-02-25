#!/bin/bash
echo Stopping containers

# Stop all containers
docker stop local-dcs-fred
docker stop local-dcs-alf
docker stop local-dcs-mock

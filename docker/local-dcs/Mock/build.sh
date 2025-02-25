#!/bin/bash

print_failure() {
    echo "Error: $1 failed. Exiting."
    exit 1
}

docker build -f Mock/Dockerfile -t alfipbus-tester ..
if [ $? -ne 0 ]; then
    print_failure "alfipbus-tester build"
fi
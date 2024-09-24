#!/bin/bash

print_failure() {
    echo "Error: $1 failed. Exiting."
    exit 1
}

docker build -f AlfIPbus/Dockerfile -t alf-ipbus AlfIPbus
if [ $? -ne 0 ]; then
    print_failure "alfipbus build"
fi


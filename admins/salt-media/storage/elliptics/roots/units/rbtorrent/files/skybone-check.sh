#!/bin/bash

if nc -z localhost 7000; then
    echo "0;skybone-mds reachable"
else
    echo "2;skybone-mds unavailable at 7000"
fi

#!/bin/bash
name='api-adapter'

nc -z localhost 8443 2>/dev/null

if [[ $? -eq 0 ]] ; then
    echo "PASSIVE-CHECK:$name;0;OK"
else
    echo "PASSIVE-CHECK:$name;2;Down"; exit
fi

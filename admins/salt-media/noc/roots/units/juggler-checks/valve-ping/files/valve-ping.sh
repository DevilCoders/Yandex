#!/bin/bash
if [ `curl -s -o /dev/null -I -w "%{http_code}" localhost:7736/ValveServer/Ping` -ne 200 ]
then
    echo "PASSIVE-CHECK:valve-ping;2;not 200 code from valve"
else
    echo "PASSIVE-CHECK:valve-ping;0;OK"
fi

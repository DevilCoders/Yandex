#!/bin/bash
name='api-gateway-envoy-in'
ret=$(curl -s -o /dev/null -w %{http_code} -k https://localhost/endpoints -H "Host: apiendpoints.local")

if [[ $ret -eq 200 ]] ; then
    echo "PASSIVE-CHECK:api-gateway-envoy-in;0;OK"
else
    echo "PASSIVE-CHECK:api-gateway-envoy-in;2;Down"; exit
fi

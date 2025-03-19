#!/usr/bin/env bash

GRPC_PORT=${GRPC_PORT:-2135}

myArray1=()
for arg in "$@";
do
  if [[ $arg =~ "grpc-port" ]]; then
     arg="--grpc-port=$GRPC_PORT"
  fi
  if [[ $arg =~ "server=grpc" ]]; then
     arg="--server=grpc://localhost:$GRPC_PORT"
  fi
  myArray1+=("$arg")
done

exec "/usr/bin/kikimr" "${myArray1[@]}"


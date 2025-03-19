#!/usr/bin/env bash

kill -9 `ps -e | grep accesservice | awk '{print $1}'`
kill -9 `ps -e | grep metadata | awk '{print $1}'`
kill -9 `ps -e | grep kikimr | awk '{print $1}'`
kill -9 `ps -e | grep blockstore | awk '{print $1}'`
kill -9 `ps -e | grep yc-snapshot | awk '{print $1}'`
kill -9 `ps -e | grep yc-disk-manager | awk '{print $1}'`

./setup.sh

./accessservice-mock.sh >logs/accessservice-mock.log 2>&1 &
./metadata-mock.sh >logs/metadata-mock.log 2>&1 &

./kikimr-configure.sh
./kikimr-format.sh
./kikimr-server.sh >logs/kikimr-server.log 2>&1 &
sleep 10
./kikimr-init.sh

./blockstore-server.sh >logs/blockstore-server.log 2>&1 &

./yc-snapshot-init.sh
./yc-snapshot.sh >logs/yc-snapshot.log 2>&1 &

./yc-disk-manager-init.sh
./yc-disk-manager.sh >logs/yc-disk-manager.log 2>&1 &

sleep infinity

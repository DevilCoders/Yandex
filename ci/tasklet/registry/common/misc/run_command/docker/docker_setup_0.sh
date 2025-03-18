#!/usr/bin/env bash

set -e

echo "Starting docker"
service docker start || (echo "Failed to start docker, may already be running.";(which journalctl && journalctl -xe))
service docker status

echo "Mounting docker network bridge6"
docker network create bridge6 --gateway 172.18.0.1 --subnet 172.18.0.0/16 --ipv6 --subnet fe00::/64 --opt "com.docker.network.bridge.enable_icc"="true" --opt "com.docker.network.bridge.enable_ip_masquerade"="true" --attachable
ip6tables -t nat -A POSTROUTING -s fe00::/64 -j MASQUERADE || echo ""

sudo chmod o+rw /var/run/docker.sock
echo "Docker is ready"

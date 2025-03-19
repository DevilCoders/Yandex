#!/bin/sh

apt-get update -qq && apt-get install -y openssh-server
service ssh start

echo "SSH is active"

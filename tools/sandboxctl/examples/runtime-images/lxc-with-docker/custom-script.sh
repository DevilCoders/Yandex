#!/bin/bash

set -xe

DIR=$(dirname $0)

$DIR/install-docker.sh

#Install extra packages
apt-get install -y xjobs zstd
install -m 0555 $DIR/lxc-with-docker/docker-load.sh /usr/bin/

pip install --upgrade pip setuptools
hash pip

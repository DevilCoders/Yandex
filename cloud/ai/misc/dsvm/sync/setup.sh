#!/bin/bash
set -ev

# for executing run:
# source setup.sh
#

sudo bash -c "./install-common-env.sh"
pushd . && source ./conda/bootstrap-conda.sh && popd
./conda/install-conda-env.sh
./jupyter/bootstrap-jupyter.sh
./docker/bootstrap-docker.sh

sudo cp -r ./examples /opt/

# change ownership
sudo chmod -R a+rwX /opt/conda
sudo chmod -R a+rwX /opt/examples
sudo chown -R root /opt

# cleanup
sudo rm -rf /tmp/*
sudo apt-get clean all
sudo rm -rf /var/lib/apt/lists/*


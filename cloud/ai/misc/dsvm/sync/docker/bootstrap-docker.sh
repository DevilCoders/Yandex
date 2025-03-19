#!/bin/bash
set -ev

# https://docs.docker.com/cs-engine/1.12/#install-on-ubuntu-1404-lts-or-1604-lts

curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"

sudo apt-get update && sudo apt-get install -y docker-ce

# to run at startup
sudo systemctl enable docker

# https://www.digitalocean.com/community/tutorials/how-to-install-and-use-docker-on-ubuntu-16-04
# sudo usermod -aG docker ${USER}
# su - ${USER}
# id -nG


# test
# docker run hello-world

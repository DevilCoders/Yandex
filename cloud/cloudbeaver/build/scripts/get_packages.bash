#!/bin/bash

set -e

# docker
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg
echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/docker-archive-keyring.gpg] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null

# yarn
curl -sL https://dl.yarnpkg.com/debian/pubkey.gpg | sudo apt-key add -
echo "deb https://dl.yarnpkg.com/debian/ stable main" | sudo tee /etc/apt/sources.list.d/yarn.list

# nodejs
curl -sL https://deb.nodesource.com/setup_14.x | sudo -E bash -

# package install
apt-get update
apt-get install -y telnet jq yandex-passport-vault-client openjdk-11-jdk maven yarn nodejs docker-ce docker-ce-cli containerd.io
npm install -g lerna

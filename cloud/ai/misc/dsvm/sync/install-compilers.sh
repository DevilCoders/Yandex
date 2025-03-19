#!/bin/bash -vx

# nodejs
# https://nodejs.org/en/download/package-manager/
curl -sL https://deb.nodesource.com/setup_9.x | sudo -E bash -
apt-get ${QUIET_MODE} install --no-install-recommends -y nodejs

# clang
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
apt-add-repository -y "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-5.0 main"
apt-get ${QUIET_MODE} update
apt-get ${QUIET_MODE} install --no-install-recommends -y clang-5.0
update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-5.0 100
update-alternatives --install /usr/bin/clang clang /usr/bin/clang-5.0 100

# test
clang++ --version
clang --version

# gcc & g++
add-apt-repository -y ppa:ubuntu-toolchain-r/test
apt-get ${QUIET_MODE} update
apt-get ${QUIET_MODE} install --no-install-recommends -y gcc-7 g++-7

update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 10
update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 10
update-alternatives --install /usr/bin/cc cc /usr/bin/gcc 30
update-alternatives --set cc /usr/bin/gcc
update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++ 30
update-alternatives --set c++ /usr/bin/g++

gcc -v
g++ -v

# go lang
wget https://dl.google.com/go/go1.10.2.linux-amd64.tar.gz
tar -xf go1.10.2.linux-amd64.tar.gz
[ -d /usr/local/go ] && sudo rm -rf /usr/local/go
sudo mv go /usr/local
export PATH=/usr/local/go/bin:${PATH}

go version

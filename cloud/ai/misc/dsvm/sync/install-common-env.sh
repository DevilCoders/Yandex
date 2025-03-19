#!/bin/bash -vx
set -e

# Reset
Color_Off='\033[0m'       # Text Reset

# Regular Colors
Black='\033[0;30m'        # Black
Red='\033[0;31m'          # Red
Green='\033[0;32m'        # Green
Yellow='\033[0;33m'       # Yellow
Blue='\033[0;34m'         # Blue
Purple='\033[0;35m'       # Purple
Cyan='\033[0;36m'         # Cyan
White='\033[0;37m'        # White

QUIET_MODE="--quiet --quiet"

apt-get ${QUIET_MODE} update
apt-get ${QUIET_MODE} install --no-install-recommends -y mc build-essential python-setuptools htop vim python-dev gawk whois cython cython3 unzip tree

# source verion control tools
apt-get ${QUIET_MODE} install --no-install-recommends -y subversion git mercurial

# dsvm.sh
buildtime=`LC_TIME=en_US.UTF-8 date -u`
cat profile.update | sudo sed "s/INSERT_BUILD_DATE_HERE/${buildtime}/" > /etc/profile.d/dsvm_init.sh

# echo
echo -e "${Green}OK${Color_Off}"



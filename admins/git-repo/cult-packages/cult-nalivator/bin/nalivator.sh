#!/bin/bash

myhost=$(hostname -f)

packages=$(curl -s --retry 3 http://c.yandex-team.ru/api/packages_on_host/${myhost}  | grep 'false' | awk '{print $1"="$3}')
cauth_packages=$(curl -s --retry 3 http://c.yandex-team.ru/api/packages_on_host/${myhost}  | grep 'false' | awk '{print $1"="$3}'| grep cauth)

apt-get -qq update

apt-get install -y ${cauth_packages} || apt-get install ${cauth_packages}

apt-get install -y ${packages} || apt-get install ${packages}


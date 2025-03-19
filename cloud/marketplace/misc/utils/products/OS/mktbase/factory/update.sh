#!/bin/bash -e

echo 'deb http://security.ubuntu.com/ubuntu xenial-security main restricted' >> /etc/apt/sources.list
echo 'deb http://security.ubuntu.com/ubuntu xenial-security universe' >> /etc/apt/sources.list
echo 'deb http://security.ubuntu.com/ubuntu xenial-security multiverse' >> /etc/apt/sources.list
 
echo 'deb http://mirror.yandex.ru/ubuntu xenial main' >> /etc/apt/sources.list
echo 'deb-src http://mirror.yandex.ru/ubuntu xenial main' >> /etc/apt/sources.list
 
echo 'deb http://mirror.yandex.ru/ubuntu xenial-updates main' >> /etc/apt/sources.list
echo 'deb-src http://mirror.yandex.ru/ubuntu xenial-updates main' >> /etc/apt/sources.list

add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu xenial stable"
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -

apt-get -y update
apt-get -y upgrade

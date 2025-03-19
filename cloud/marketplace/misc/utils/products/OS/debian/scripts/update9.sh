#!/bin/bash -e

#echo 'deb http://security.debian.org/ stretch/updates main' >> /etc/apt/sources.list
#echo 'deb-src http://security.debian.org/ stretch/updates main' >> /etc/apt/sources.list
 
echo 'deb http://mirror.yandex.ru/debian stretch main' >> /etc/apt/sources.list
echo 'deb-src http://mirror.yandex.ru/debian stretch main' >> /etc/apt/sources.list
 
echo 'deb http://mirror.yandex.ru/debian stretch-updates main' >> /etc/apt/sources.list
echo 'deb-src http://mirror.yandex.ru/debian stretch-updates main' >> /etc/apt/sources.list

apt-get -y update
apt-get -y upgrade

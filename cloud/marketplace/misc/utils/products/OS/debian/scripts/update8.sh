#!/bin/bash -e

#echo 'deb http://security.debian.org/ jessie/updates main' >> /etc/apt/sources.list
#echo 'deb-src http://security.debian.org/ jessie/updates main' >> /etc/apt/sources.list
 
echo 'deb http://mirror.yandex.ru/debian jessie main' >> /etc/apt/sources.list
echo 'deb-src http://mirror.yandex.ru/debian jessie main' >> /etc/apt/sources.list
 
echo 'deb http://mirror.yandex.ru/debian jessie-backports main' >> /etc/apt/sources.list
echo 'deb-src http://mirror.yandex.ru/debian jessie-backports main' >> /etc/apt/sources.list

echo 'deb http://mirror.yandex.ru/debian jessie-updates main' >> /etc/apt/sources.list
echo 'deb-src http://mirror.yandex.ru/debian jessie-updates main' >> /etc/apt/sources.list

apt-get -y update
apt-get -y upgrade

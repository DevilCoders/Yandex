#!/usr/bin/env bash

set -e

sudo touch /etc/apt/sources.list.d/common.list

echo 'deb http://common.dist.yandex.ru/common stable/all/' | sudo tee -a /etc/apt/sources.list.d/common.list
echo 'deb http://common.dist.yandex.ru/common stable/amd64/' | sudo tee -a /etc/apt/sources.list.d/common.list

sudo apt update -qq
sudo apt -y install yandex-arc-launcher

mkdir -p ~/ya/arcadia
mkdir -p ~/ya/store

echo 'user_allow_other' | sudo tee -a /etc/fuse.conf
arc token
arc mount -m ~/ya/arcadia -S ~/ya/store --allow-other

mkdir -p ~/.ya

# /dev/sdc1 /dev/sde1 /dev/sdf1 are HDDs those are not used by kikimr
# you can safely build raid on top of them to store build artifacts

sudo mdadm --create --verbose /dev/md127 --level=0 --raid-devices=3 /dev/sdc1 /dev/sde1 /dev/sdf1
sudo mkfs.ext4 -F /dev/md127
echo /dev/md127 $HOME/.ya/  ext4  defaults,errors=remount-ro 0 2 | sudo tee -a /etc/fstab
sudo mount -a



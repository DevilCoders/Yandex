#!/bin/bash

# check for Yandex Disk Repo

if [ ! -d /storage/yandexrepo/yandex-disk ]; then
    echo "2;Yandex Disk Repository dissapeared"
else
    echo "0;ok"
fi

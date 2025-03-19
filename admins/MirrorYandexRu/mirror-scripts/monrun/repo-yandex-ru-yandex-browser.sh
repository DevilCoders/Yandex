#!/bin/bash

# check for Yandex Browser Repo

if [ ! -d /storage/yandexrepo/yandex-browser ]; then
    echo "2;Yandex Browser Repository dissapeared"
else
    echo "0;ok"
fi

#!/usr/bin/env bash

if exec groups | fgrep -qw docker ; then
    echo "User is in docker group"
else
    echo "Adding user ${USER} to docker group"
    sudo usermod -a -G docker "$USER" || exit 1
    echo "Now you need to logout and login again"
    exit 1
fi

docker pull cr.yandex/crp7nvlkttssi7kapoho/iot/tank:{{ tank_application_version }} || exit 1

#!/bin/bash

DOCKERFILE_DIR="."

wget "https://proxy.sandbox.yandex-team.ru/1253213295" -O cbirdaemon2_light
#ссылка на ресурс https://sandbox.yandex-team.ru/resource/1253213295/view
ya tools strip cbirdaemon2_light

sudo docker build -t cbirdaemon $DOCKERFILE_DIR
sudo docker save cbirdaemon > cbirdaemon.tar

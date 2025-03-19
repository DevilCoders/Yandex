#!/bin/bash
apt update --allow-insecure-repositories --allow-unauthenticated
apt install --yes --force-yes \
  python3.7-dev python3.7-venv python3-venv python3-pip \
  libopenmpi-dev openmpi-bin cython3 build-essential g++ \
  libssl-dev
  
apt install --yes yandex-internal-root-ca ca-certificates

mkdir -p ~/.clickhouse-client /usr/local/share/ca-certificates/Yandex && \
wget "https://crls.yandex.net/allCAs.pem" -O /usr/local/share/ca-certificates/Yandex/YandexInternalRootCA.crt && \
wget "https://storage.yandexcloud.net/mdb/clickhouse-client.conf.example" -O ~/.clickhouse-client/config.xml

update-ca-certificates

python3.7 -m pip install --upgrade pip
python3.7 -m pip install requests==2.23.0
python3.7 -m pip install -i https://pypi.yandex-team.ru/simple/ \
 yandex-yt \
 yandex-passport-vault-client \
 yandex-tracker-client \
 yql \
 yandex-yt-transfer-manager-client \
 yandex-spyt
  
python3.7 -m pip install --no-cache-dir -i https://pypi.yandex-team.ru/simple/ \
  clan_tools 


python3.7 -m pip install --no-cache-dir  \
  pymorphy2==0.9.1 \
  pandas==1.2.4 \
  Flask==2.0.1 \
  click==7.1.2 \
  protobuf==3.20.1 \
  tensorflow==2.4.1



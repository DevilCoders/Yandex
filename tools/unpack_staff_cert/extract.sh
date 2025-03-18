#!/usr/bin/env bash

set -e

if [ -z $USER ] ; then
    echo "Error: \$USER environment is not set"
    exit 1
fi

echo "Extracting certificate, please input password from https://crt.yandex-team.ru/request/"
openssl pkcs12 -in ${USER}@ld.yandex.ru.pfx -clcerts -nokeys -out  ${USER}.crt

# set some password that will be written into password.txt
echo "Extracting certificate, please input password from https://crt.yandex-team.ru/request/"
echo "Please set key encryption password when prompted"
openssl pkcs12 -in ${USER}@ld.yandex.ru.pfx -nocerts -out ${USER}-encrypted.key

echo "Please set the same key encryption password for RSA-converted key"
openssl rsa -des3 -in ${USER}-encrypted.key  -out ${USER}.key

echo "Setting permissions"
# chmod 600 password.txt
chmod 600 ${USER}.key

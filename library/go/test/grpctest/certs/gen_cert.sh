#!/bin/bash
# Regenerate the self-signed certificate for local host.

openssl req \
    -x509 \
    -newkey rsa:2048 \
    -sha256 \
    -days 3650 \
    -nodes \
    -keyout localhost.key \
    -out localhost.crt \
    -subj '/C=RU/L=Moscow/O=Yandex/OU=Infrastructure/CN=localhost' \
    -extensions sans \
    -config ssl.conf

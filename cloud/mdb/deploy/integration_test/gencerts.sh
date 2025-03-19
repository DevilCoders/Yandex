#!/bin/bash

# TODO: reimplement in Go

set -e

mkdir -p certs

# generate CA
openssl genrsa -out certs/cert.pem 4096
openssl req -x509 -new -key certs/cert.pem -out certs/key.pem -days 730 -subj "/C=RU"
mkdir -p images/deploy-test/config
cp certs/key.pem images/deploy-test/config/allCAs.pem
cp certs/key.pem images/salt-master/srv/salt/allCAs.pem
cp certs/key.pem images/salt-master/config/allCAs.pem
cp certs/key.pem images/mdb-deploy-api/config/allCAs.pem
cp certs/key.pem images/salt-minion/config/allCAs.pem

# generate salt-api nginx keys and certs
openssl dhparam -out images/salt-master/config/dhparam.pem 1024
openssl genrsa -out images/salt-master/config/salt-api.key 4096
openssl req -new -key images/salt-master/config/salt-api.key -subj "/C=RU/CN=*.salt-master" \
    -reqexts SAN -extensions SAN -config <(cat /etc/ssl/openssl.cnf <(printf "[SAN]\nsubjectAltName=DNS:*.salt-master")) \
| openssl x509 -req -out images/salt-master/config/salt-api.pem \
    -CAkey certs/cert.pem -CA certs/key.pem -days 365 -CAcreateserial \
    -extfile <(printf "subjectAltName=DNS:*.salt-master")

# generate salt-master key pair
openssl genrsa -out images/salt-master/config/master.pem 4096
openssl rsa -in images/salt-master/config/master.pem -pubout > images/salt-master/config/master.pub

# generate salt-master message signing key pair
openssl genrsa -out images/salt-master/config/master_sign.pem 4096
openssl rsa -in images/salt-master/config/master_sign.pem -pubout > images/salt-master/config/master_sign.pub
cp images/salt-master/config/master_sign.pub images/salt-minion/config/

# generate mdb-deploy-api nginx keys and certs
openssl dhparam -out images/mdb-deploy-api/config/dhparam.pem 1024
openssl genrsa -out images/mdb-deploy-api/config/mdb-deploy-api.key 4096
openssl req -new -key images/mdb-deploy-api/config/mdb-deploy-api.key -subj "/C=RU/CN=mdb-deploy-api" \
    -reqexts SAN -extensions SAN -config <(cat /etc/ssl/openssl.cnf <(printf "[SAN]\nsubjectAltName=DNS:mdb-deploy-api")) \
| openssl x509 -req -out images/mdb-deploy-api/config/mdb-deploy-api.pem \
    -CAkey certs/cert.pem -CA certs/key.pem -days 365 -CAcreateserial \
    -extfile <(printf "subjectAltName=DNS:mdb-deploy-api")

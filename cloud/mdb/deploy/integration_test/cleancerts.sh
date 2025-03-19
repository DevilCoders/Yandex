#!/bin/bash

set -e

rm -rf certs
rm -rf images/deploy-test/config/allCAs.pem
rm -rf images/salt-master/srv/salt/allCAs.pem
rm -rf images/salt-master/config/allCAs.pem
rm -rf images/mdb-deploy-api/config/allCAs.pem
rm -rf images/salt-minion/config/allCAs.pem

rm -rf images/salt-master/config/dhparam.pem
rm -rf images/salt-master/config/salt-api.key
rm -rf images/salt-master/config/salt-api.pem

rm -rf images/salt-master/config/master.pem
rm -rf images/salt-master/config/master.pub

rm -rf images/salt-master/config/master_sign.pem
rm -rf images/salt-master/config/master_sign.pub
rm -rf images/salt-minion/config/master_sign.pub

rm -rf images/mdb-deploy-api/config/dhparam.pem
rm -rf images/mdb-deploy-api/config/mdb-deploy-api.key
rm -rf images/mdb-deploy-api/config/mdb-deploy-api.pem

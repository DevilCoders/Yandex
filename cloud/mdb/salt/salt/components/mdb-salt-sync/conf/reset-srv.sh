#!/bin/bash

set -e

# TODO: either move to mdb-salt-sync or use repos.sls

/opt/yandex/mdb-salt-sync/fix-permissions.sh

sudo -u robot-pgaas-deploy svn revert --recursive /srv
sudo -u robot-pgaas-deploy git -C /srv/pillar/private reset --hard HEAD
sudo -u robot-pgaas-deploy svn up /srv/salt/components/pg-code
sudo -u robot-pgaas-deploy svn up /srv/salt/components/pg-code/dbaas_metadb
sudo -u robot-pgaas-deploy svn up /srv/salt/components/pg-code/deploydb
sudo -u robot-pgaas-deploy svn up /srv/salt/components/pg-code/secretsdb
sudo -u robot-pgaas-deploy svn up /srv/salt/components/pg-code/cmsdb
sudo -u robot-pgaas-deploy svn up /srv/salt/components/pg-code/katandb
sudo -u robot-pgaas-deploy git -C /srv/salt/components/pg-code/dbm reset --hard HEAD

/opt/yandex/mdb-salt-sync/fix-permissions.sh

#!/bin/bash

set -e

# Porto sync - all porto salt-master switched to s3 images

RSYNC_BASE='rsync -avz --exclude pillar/private/* --exclude .svn --exclude .git'
# Compute sync. Just v1 salt-master.
for TARGET in salt-dbaas01k.yandexcloud.net
do
    # 146th level magic to achieve 'atomic' update for remote salts
    # without delete, without softlinks
    echo "$(date "+%Y-%m-%d %H:%M:%S") Syncing $TARGET without delete, without softlinks"
    $RSYNC_BASE --exclude envs/qa --exclude envs/load --exclude envs/prod --exclude envs/compute-prod \
        /srv $TARGET:/
    # without delete
    echo "$(date "+%Y-%m-%d %H:%M:%S") Syncing $TARGET without delete"
    $RSYNC_BASE /srv $TARGET:/
    # with everything
    echo "$(date "+%Y-%m-%d %H:%M:%S") Syncing $TARGET with everything"
    $RSYNC_BASE --delete /srv $TARGET:/

    # pillar/private/external
    echo "$(date "+%Y-%m-%d %H:%M:%S") Syncing $TARGET with pillar/private/external"
    rsync -avz --delete /srv/pillar/private/external $TARGET:/srv/pillar/private/

    # pillar/private/gpg
    echo "$(date "+%Y-%m-%d %H:%M:%S") Syncing $TARGET with pillar/private/gpg"
    rsync -avz /srv/pillar/private/gpg/secrets-db-prod01 /srv/pillar/private/gpg/secrets-db-preprod01 $TARGET:/srv/pillar/private/gpg

    # pillar/private/pg/tls
    echo "$(date "+%Y-%m-%d %H:%M:%S") Syncing $TARGET with pillar/private/pg/tls"
    rsync -avz --relative \
    /srv/pillar/private/./pg/tls/prod/secrets-db-prod* \
    /srv/pillar/private/./pg/tls/prod/secrets-db-preprod* \
    /srv/pillar/private/./pg/tls/prod/allCAs.pem \
    $TARGET:/srv/pillar/private
done

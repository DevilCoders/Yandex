#!/bin/bash

# Move zookeeper data to correct location: https://st.yandex-team.ru/MDB-10345

set -e

DRY_RUN=$1
if [ "$DRY_RUN" != false ] && [ "$DRY_RUN" != true ] ; then
  echo "only 'false' and 'true' accepted as first argument (dry run mode)"
  exit 1
fi

shift 1
for CLUSTER_ID in "$@"
do
  echo "Processing cluster $CLUSTER_ID"
  HAS_ZK_SUBCLUSTER=$(dbaas pillar get-key -c "$CLUSTER_ID" data:kafka:has_zk_subcluster)

  if [ "$HAS_ZK_SUBCLUSTER" == "true" ]
  then
    echo "Cluster $CLUSTER_ID has dedicated zookeeper hosts"
    HOSTS=$(dbaas host list -c $CLUSTER_ID | grep '{zk}' | awk '{print $1};' | tr '\n' ' ')
    OLD_DIR="/data/zookeper"
    NEW_DIR="/var/lib/zookeeper"
  else
    echo "Cluster $CLUSTER_ID does not have dedicated zookeeper hosts"
    HOSTS=$(dbaas host list -c $CLUSTER_ID | grep '{kafka_cluster}' | awk '{print $1};' | tr '\n' ' ')
    OLD_DIR="/data/zookeper"
    NEW_DIR="/data/zookeeper"
  fi

  DATADIR=$(dbaas pillar get-key -c $CLUSTER_ID data:zk:config:dataDir)
  if [ "$DATADIR" != "\"$NEW_DIR\"" ]
  then
    if [ "$DRY_RUN" = false ] ; then
      echo "Acquiring lock ..."
      ssh root@mlock01f.yandexcloud.net "mlock create --holder vlbel --reason https://st.yandex-team.ru/MDB-10345 --lock-id vlbel-$CLUSTER_ID $HOSTS"
    fi

    for HOST in $HOSTS
    do
      echo "Processing zookeeper host $HOST"

      REAL_DATA_DIR=$(ssh "root@$HOST" "cat /etc/zookeeper/conf/zoo.cfg | grep dataDir | sed 's/dataDir=//g'")
      if [ "$REAL_DATA_DIR" == "$OLD_DIR" ] ; then
        echo "Data dir is as expected: $REAL_DATA_DIR"
      else
        echo "[!!! ACHTUNG !!!] Real data dir $REAL_DATA_DIR differs from expected $OLD_DIR"
        exit 1
      fi

      if [ "$DRY_RUN" = false ] ; then
        # replace "/" with "\/" to use in sed substitute expression
        OLD_DIR_QUOTED=$(echo $OLD_DIR | sed 's/\//\\\//g')
        NEW_DIR_QUOTED=$(echo $NEW_DIR | sed 's/\//\\\//g')

        ssh "root@$HOST" "set -x; service zookeeper stop &&
          mkdir -p $NEW_DIR &&
          chown -R zookeeper:zookeeper $NEW_DIR &&
          cp -rp $OLD_DIR/* $NEW_DIR/ &&
          sed -i -e 's/$OLD_DIR_QUOTED/$NEW_DIR_QUOTED/g' /etc/zookeeper/conf/zoo.cfg &&
          sed -i -e 's/$OLD_DIR_QUOTED/$NEW_DIR_QUOTED/g' /lib/systemd/system/zookeeper.service &&
          sed -i -e 's/$OLD_DIR_QUOTED/$NEW_DIR_QUOTED/g' /etc/cron.d/zk-rotate-xact-logs &&
          systemctl daemon-reload &&
          service zookeeper start"
      fi
    done

    if [ "$DRY_RUN" = false ] ; then
      echo "Updating pillar ..."
      dbaas pillar set-key -c $CLUSTER_ID data:zk:config:dataDir "\"$NEW_DIR\""

      echo "Releasing lock ..."
      ssh root@mlock01f.yandexcloud.net "mlock release --lock-id vlbel-$CLUSTER_ID"
    fi
  else
    echo "Cluster $CLUSTER_ID already has zookeeper data at correct location $NEW_DIR"
  fi

  echo "Processed cluster $CLUSTER_ID"
  echo "============================="
  echo ""
done

#!/bin/sh

THL_PORT=2112

envir=$(cat /etc/yandex/environment.type)

if [ $envir == "testing" ];then 
	REMOTE_MASTER="test-mdbs.kp.yandex.net"
elif [ $envir == "stress" ];then 
	REMOTE_MASTER="kp-dbs01h.load.jkp.yandex.net"
elif [ $envir == "production" ];then 
	REMOTE_MASTER="export01-dbs.kp.yandex.net"
else
	echo "Not supported environment $envir"
	exit 1
fi

if [ -d /opt/replicator ]; then
	action="update";
else 
	action="install";
fi

./tools/tpm $action  mongodb \
  --reset \
  --topology=master-slave \
  --role=slave \
  --master=localhost \
  --datasource-port=27017 \
  --datasource-type=mongodb \
  --datasource-host=localhost \
  --home-directory=/opt/replicator \
  --enable-heterogenous-slave=true \
  --skip-validation-check=InstallerMasterSlaveCheck \
  --skip-validation-check=HostsFileCheck \
  --skip-validation-check=ReplicationServicePipelines \
  --skip-validation-check=ModifiedConfigurationFilesCheck \
  --master-thl-host=$REMOTE_MASTER \
  --master-thl-port=$THL_PORT \
  --start=false 

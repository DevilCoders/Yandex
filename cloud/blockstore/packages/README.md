# Where are my configs & packages?

Y.Cloud PROD & PRE-PROD:
* configs: https://a.yandex-team.ru/arc/trunk/arcadia/kikimr/production/configs/yandex-cloud
* packages: https://a.yandex-team.ru/arc/trunk/arcadia/kikimr/production/packages/yandex-cloud

Y.Cloud TESTING:
* configs: https://a.yandex-team.ru/arc/trunk/arcadia/kikimr/testing/configs/yandex-cloud
* packages: https://a.yandex-team.ru/arc/trunk/arcadia/kikimr/testing/packages/yandex-cloud

KiKiMR DEV:
* configs: https://a.yandex-team.ru/arc/trunk/arcadia/kikimr/testing/configs/nbs
* packages: https://a.yandex-team.ru/arc/trunk/arcadia/kikimr/testing/packages/nbs

# How to apply changes?

\# Build KiKiMR configure tool
```bash
vskipin@rtmr-dev00:~/arc/trunk$ ymake arcadia/kikimr/tools/cfg/
```

\# Locate YAML config and apply your changes
```bash
vskipin@rtmr-dev00:~/arc/trunk$ vi arcadia/kikimr/testing/configs/cloud/man_dev/cluster.yaml
```

\# Generate configs
Execute the following operation for each cluster (man_dev, man_test, {prod,preprod,testing}-{sas,vla,myt})
```bash
vskipin@rtmr-dev00:~/arc/trunk$ ./ybuild/release/bin/kikimr_configure cfg --nbs --grpc-endpoint grpc://localhost:2135 arcadia/kikimr/testing/configs/cloud/man_dev/cluster.yaml ybuild/release/bin/kikimr arcadia/kikimr/testing/configs/cloud/man_dev/nbs/cfg/
```

\# Commit your changes
```bash
vskipin@rtmr-dev00:~/arc/trunk$ svn ci -m "update configs"
```

# How to deploy? (DEV cluster)

Build KiKiMR slice update tool
```bash
vskipin@rtmr-dev00:~/arc/trunk$ ymake arcadia/kikimr/tools/kikimr_slice/
```

OR just build everything alltogether
```bash
vskipin@rtmr-dev00:~/arc/trunk$ ymake arcadia/cloud/blockstore/buildall/
```

Update DEV cluster
```bash
vskipin@rtmr-dev00:~/arc/trunk$ ./ybuild/release/kikimr/tools/kikimr_slice/kikimr_slice update arcadia/kikimr/testing/configs/cloud/man_dev/cluster.yaml --kikimr ybuild/release/kikimr/driver/kikimr --nbs-server ybuild/release/cloud/blockstore/daemon/blockstore-server --nbs-plugin ybuild/release/cloud/vm/blockstore/libblockstore-plugin.so
```

# How to deploy? (Y.Cloud)

\# Build your packages and publish them into Y.Cloud diststore
* https://teamcity.yandex-team.ru/viewType.html?buildTypeId=NetworkBlockStore_Trunk_Packages_BuildBinPackages
* https://teamcity.yandex-team.ru/viewType.html?buildTypeId=NetworkBlockStore_Trunk_Packages_BuildConfigPackages
Package ids (e.g. 5288935.trunk.464587460) can be found in the build task logs. The task publishes packages into search repositories. To move them into ycloud repositories you need to follow the instructions from this page: https://wiki.yandex-team.ru/kikimr/projects/yandexcloud/cloud-oncall-howto/cluster-update/#paketyirepozitorii

\# Then modify package assignment in Z2 configuration
https://z2.yandex-team.ru/configs

IMPORTANT: Config package versions should be edited via ```YCLOUD_{PREPROD,PROD}_NBS_{SAS,VLA,MYT}_CONF``` even though the "Update on workers" action should always be initiated via ```YCLOUD_{PREPROD,PROD}_NBS_{SAS,VLA,MYT}```

After the update process completes you can use the rolling_restart tool to restart NBS services.

\# Build KiKiMR rolling restart tool
```bash
vskipin@rtmr-dev00:~/arc/trunk$ ymake arcadia/kikimr/tools/rolling_restart/bin/
```

\# Restart services

IMPORTANT: First of all you should manually restart NBS on a single host and check that everything is fine. After that run the rolling_restart tool as follows:
```bash
vskipin@rtmr-dev00:~/arc/trunk$ ./ybuild/release/bin/kikimr_rolling_restart --ssh-cmd yc-ssh --service cloud-nbs --grpc --addr ydbproxy-vla.cloud-preprod.yandex.net
```

# How to rollback? (Y.Cloud)
There is no explicit rollback command - you will need to change package versions to the old ones in Z2 (using the 'force' flag) and then follow the instructions from the previous section of this README.

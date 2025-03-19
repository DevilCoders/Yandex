#!/bin/bash -x
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -ex

usage() {
  echo "
usage: $0 <options>
  Required not-so-options:
     --build-dir=DIR             path to knox dist.dir
     --source-dir=DIR            path to package shared files dir
     --prefix=PREFIX             path to install into

  Optional options:
     --doc-dir=DIR               path to install docs into [/usr/share/doc/knox]
     --lib-dir=DIR               path to install knox home [/usr/lib/knox]
     --installed-lib-dir=DIR     path where lib-dir will end up on target system
     --bin-dir=DIR               path to install bins [/usr/bin]
     --examples-dir=DIR          path to install examples [doc-dir/examples]
     ... [ see source for more similar options ]
  "
  exit 1
}

OPTS=$(getopt \
  -n $0 \
  -o '' \
  -l 'prefix:' \
  -l 'distro-dir:' \
  -l 'local-conf-dir:' \
  -l 'conf-dir:' \
  -l 'bin-dir:' \
  -l 'lib-dir:' \
  -l 'templates-dir:' \
  -l 'pids-dir:' \
  -l 'data-dir:' \
  -l 'logs-dir:' \
  -l 'doc-dir:' \
  -l 'initd-dir:' \
  -l 'dep-dir:' \
  -l 'source-dir:' \
  -l 'build-dir:' -- "$@")

if [ $? != 0 ] ; then
    usage
fi

eval set -- "$OPTS"
while true ; do
    case "$1" in
        --prefix)
        PREFIX=$2 ; shift 2
        ;;
        --distro-dir)
        DISTRO_DIR=$2 ; shift 2
        ;;
        --bin-dir)
        BIN_DIR=$2 ; shift 2
        ;;
        --conf-dir)
        CONF_DIR=$2 ; shift 2
        ;;
        --lib-dir)
        LIB_DIR=$2 ; shift 2
        ;;
        --templates-dir)
        TEMPLATES_DIR=$2 ; shift 2
        ;;
        --pids-dir)
        PIDS_DIR=$2 ; shift 2
        ;;
        --data-dir)
        DATA_DIR=$2 ; shift 2
        ;;
        --logs-dir)
        LOGS_DIR=$2 ; shift 2
        ;;
        --doc-dir)
        DOC_DIR=$2 ; shift 2
        ;;
        --dep-dir)
        DEP_DIR=$2 ; shift 2
        ;;
        --source-dir)
        SOURCE_DIR=$2 ; shift 2
        ;;
        --build-dir)
        BUILD_DIR=$2 ; shift 2
        ;;
        --)
        shift ; break
        ;;
        *)
        echo "Unknown option: $1"
        usage
        exit 1
        ;;
    esac
done

for var in PREFIX BUILD_DIR; do
  if [ -z "$(eval "echo \$$var")" ]; then
    echo Missing param: $var
    usage
  fi
done

BIN_DIR=${BIN_DIR:-/usr/lib/knox/bin}
CONF_DIR=${CONF_DIR:-/etc/knox/conf.dist}
LIB_DIR=${LIB_DIR:-/usr/lib/knox/lib}
TEMPLATES_DIR=${TEMPLATES_DIR:-/usr/lib/knox/templates}
PIDS_DIR=${PIDS_DIR:-/var/run/knox}
DATA_DIR=${DATA_DIR:-/usr/lib/knox/data}
LOGS_DIR=${LOGS_DIR:-/var/log/knox}
DOC_DIR=${DOC_DIR:-/usr/share/doc/knox}
DEP_DIR=${DEP_DIR:-/usr/lib/knox/dep}
SYSTEMD_UNIT_DIR=${SYSTEMD_UNIT_DIR:-$PREFIX/lib/systemd/system}

TEMPORARY_SERVICES_DIR=${TEMPORARY_SERVICES_DIR:-${BUILD_DIR}/tmp/services}

# bin
install -d -m 0755 $PREFIX/$BIN_DIR
cp -r ${BUILD_DIR}/gateway-shell-release/home/bin/* $PREFIX/$BIN_DIR
cp -r ${BUILD_DIR}/gateway-release-common/home/bin/* $PREFIX/$BIN_DIR
cp -r ${BUILD_DIR}/gateway-release/home/bin/* $PREFIX/$BIN_DIR  # gateway.sh

for script in ${PREFIX}/${BIN_DIR}/*.sh; do
    chmod 755 $script
done

# conf
# Move the configuration files to the correct location
install -d -m 0755 $PREFIX/$CONF_DIR
ln -s $CONF_DIR $PREFIX/usr/lib/knox/conf
cp -r $BUILD_DIR/gateway-shell-release/home/conf/* ${PREFIX}/${CONF_DIR}
cp -r $BUILD_DIR/gateway-release/home/conf/* ${PREFIX}/${CONF_DIR}

# Move knox-env.sh from source to /etc and concatinate with package knox-env.sh
mv $PREFIX/$BIN_DIR/knox-env.sh $PREFIX/$CONF_DIR/knox-env.sh
chmod +x $PREFIX/$CONF_DIR/knox-env.sh
cat $SOURCE_DIR/knox-env.sh >> $PREFIX/$CONF_DIR/knox-env.sh
ln -sf $CONF_DIR/knox-env.sh $PREFIX/$BIN_DIR/knox-env.sh

# Move gateway-site.xml, gateway-log4j.properties from source to /etc
cp $SOURCE_DIR/gateway-site.xml $PREFIX/$CONF_DIR/gateway-site.xml
cp $SOURCE_DIR/gateway-log4j.properties $PREFIX/$CONF_DIR/gateway-log4j.properties

# Use gateway.cfg & knoxcli.cfg from package
cp $SOURCE_DIR/gateway.cfg $PREFIX/$CONF_DIR/gateway.cfg
ln -sf $CONF_DIR/gateway.cfg $PREFIX/$BIN_DIR/gateway.cfg
cp $SOURCE_DIR/knoxcli.cfg $PREFIX/$CONF_DIR/knoxcli.cfg
ln -sf $CONF_DIR/knoxcli.cfg $PREFIX/$BIN_DIR/knoxcli.cfg

# lib
install -d -m 0755 $PREFIX/$LIB_DIR
ls -1 ${BUILD_DIR}/*/target/*.jar | xargs -I {} cp {} ${PREFIX}/${LIB_DIR}/

#systemd units
install -d -m 0755 ${SYSTEMD_UNIT_DIR}
cp ${DISTRO_DIR}/*.service ${SYSTEMD_UNIT_DIR}/

# Create version-less symlinks to offer integration point with other projects
for DIR in ${PREFIX}/${LIB_DIR} ; do
  (cd $DIR &&
   for j in gateway-*.jar; do
     if [[ $j =~ gateway-(.*)-$KNOX_VERSION.jar ]]; then
       name=${BASH_REMATCH[1]}
       ln -s $j gateway-$name.jar
     fi
   done
   for j in knox-*.jar; do
     if [[ $j =~ knox-(.*)-$KNOX_VERSION.jar ]]; then
       name=${BASH_REMATCH[1]}
       ln -s $j knox-$name.jar
     fi
   done)
done

# Move some libraries with launchers to bin dir
cp $PREFIX/$LIB_DIR/gateway-server-launcher.jar $PREFIX/$BIN_DIR/gateway.jar
cp $PREFIX/$LIB_DIR/knox-cli-launcher.jar $PREFIX/$BIN_DIR/knoxcli.jar


# ext
install -d -m 0755 $PREFIX/$EXT_DIR
cp -r ${BUILD_DIR}/gateway-release/home/ext/* $PREFIX/$EXT_DIR

# templates
install -d -m 0755 $PREFIX/$TEMPLATES_DIR
cp -r ${BUILD_DIR}/gateway-release/home/templates/* $PREFIX/$TEMPLATES_DIR

# data
install -d -m 0755 $PREFIX/$DATA_DIR
cp -r ${BUILD_DIR}/gateway-release/home/data/* $PREFIX/$DATA_DIR
install -d -m 0755 $PREFIX/$DATA_DIR/services
install -d -m 0755 $TEMPORARY_SERVICES_DIR
cp -r ${BUILD_DIR}/gateway-service-definitions/target/classes/services/* $TEMPORARY_SERVICES_DIR
install -d -m 0700 $PREFIX/$DATA_DIR/security

# Create symlink to current version of the service
install -d -m 0755 $PREFIX/$DATA_DIR/services/hbase
cp -r $TEMPORARY_SERVICES_DIR/hbase/0.98.0  $PREFIX/$DATA_DIR/services/hbase/0.98.0
install -d -m 0755 $PREFIX/$DATA_DIR/services/hdfsui
cp -r $TEMPORARY_SERVICES_DIR/hdfsui/2.7.0  $PREFIX/$DATA_DIR/services/hdfsui/2.7.0
install -d -m 0755 $PREFIX/$DATA_DIR/services/hive
cp -r $TEMPORARY_SERVICES_DIR/hive/0.13.0  $PREFIX/$DATA_DIR/services/hive/0.13.0
install -d -m 0755 $PREFIX/$DATA_DIR/services/hue
cp -r $TEMPORARY_SERVICES_DIR/hue/1.0.0  $PREFIX/$DATA_DIR/services/hue/1.0.0
install -d -m 0755 $PREFIX/$DATA_DIR/services/jobhistoryui
cp -r $TEMPORARY_SERVICES_DIR/jobhistoryui/2.7.0  $PREFIX/$DATA_DIR/services/jobhistoryui/2.7.0
install -d -m 0755 $PREFIX/$DATA_DIR/services/nodemanagerui
cp -r $TEMPORARY_SERVICES_DIR/nodemanagerui/2.7.1  $PREFIX/$DATA_DIR/services/nodemanagerui/2.7.1
install -d -m 0755 $PREFIX/$DATA_DIR/services/oozie
cp -r $TEMPORARY_SERVICES_DIR/oozie/4.0.0  $PREFIX/$DATA_DIR/services/oozie/4.0.0
install -d -m 0755 $PREFIX/$DATA_DIR/services/oozieui
cp -r $TEMPORARY_SERVICES_DIR/oozieui/4.2.0  $PREFIX/$DATA_DIR/services/oozieui/4.2.0
install -d -m 0755 $PREFIX/$DATA_DIR/services/sparkhistoryui
cp -r $TEMPORARY_SERVICES_DIR/sparkhistoryui/2.3.0  $PREFIX/$DATA_DIR/services/sparkhistoryui/2.3.0
install -d -m 0755 $PREFIX/$DATA_DIR/services/yarn-rm
cp -r $TEMPORARY_SERVICES_DIR/yarn-rm/2.5.0  $PREFIX/$DATA_DIR/services/yarn-rm/2.5.0
install -d -m 0755 $PREFIX/$DATA_DIR/services/yarnui
cp -r $TEMPORARY_SERVICES_DIR/yarnui/2.7.0  $PREFIX/$DATA_DIR/services/yarnui/2.7.0
install -d -m 0755 $PREFIX/$DATA_DIR/services/zeppelinui
cp -r $TEMPORARY_SERVICES_DIR/zeppelinui/0.8.1  $PREFIX/$DATA_DIR/services/zeppelinui/0.8.1
install -d -m 0755 $PREFIX/$DATA_DIR/services/zeppelinws
cp -r $TEMPORARY_SERVICES_DIR/zeppelinws/0.8.1  $PREFIX/$DATA_DIR/services/zeppelinws/0.8.1
install -d -m 0755 $PREFIX/$DATA_DIR/services/hbaseui
cp -r $TEMPORARY_SERVICES_DIR/hbaseui/1.1.0  $PREFIX/$DATA_DIR/services/hbaseui/1.1.0

# docs
# install -d -m 0755 $PREFIX/$DOC_DIR
# cp  ${BUILD_DIR}/README $PREFIX/$DOC_DIR/

# dep
install -d -m 0755 $PREFIX/$DEP_DIR
ls -1 ${BUILD_DIR}/*/target/dependency/*.jar | xargs -I {} cp {} ${PREFIX}/${DEP_DIR}/

# logs
install -d -m 0755 $PREFIX/$LOGS_DIR
ln -sf $LOGS_DIR $PREFIX/usr/lib/knox/logs

# pids
install -d -m 07555 $PREFIX/$PIDS_DIR

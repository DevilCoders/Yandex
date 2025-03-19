#!/bin/sh -e

TARGET=$1
DIR=$2
PACKAGE=$3
PROJECT=$4

mkdir -p $TARGET/etc/yandex/$DIR
mkdir -p $TARGET/etc/zabbix/conf.d
mkdir -p $TARGET/etc/monrun/conf.d
mkdir -p $TARGET/etc/ubic/service
mkdir -p $TARGET/usr/lib/yandex/$DIR
mkdir -p $TARGET/usr/lib/yandex/$DIR/bin
mkdir -p $TARGET/usr/bin
mkdir -p $TARGET/etc/cron.d

cp -rl *zookeeper-*/conf $TARGET/etc/yandex/$DIR/
rm -rf *zookeeper-*/conf

cp -rl *zookeeper-*/* $TARGET/usr/lib/yandex/$DIR/

ln -s /etc/yandex/$DIR $TARGET/usr/lib/yandex/$DIR/conf
ln -s /usr/lib/yandex/$DIR/bin/zkCli.sh $TARGET/usr/bin/zk-${PROJECT}

cp src/*.sh $TARGET/usr/lib/yandex/$DIR/bin/
cp src/checks/* $TARGET/usr/lib/yandex/$DIR/bin/
cp zoo.cfg.* $TARGET/etc/yandex/$DIR/
cp rest.cfg.* $TARGET/etc/yandex/$DIR/ || echo "No rest.cfg files to install."
cp src/*.properties $TARGET/etc/yandex/$DIR/
cp src/zabbix.conf $TARGET/etc/zabbix/conf.d/$PACKAGE
cp src/monrun/zookeeper.conf $TARGET/etc/monrun/conf.d/${PACKAGE}.conf
cp src/cron $TARGET/etc/cron.d/$PACKAGE
cp src/ubic-zk.pl $TARGET/etc/ubic/service/$PACKAGE

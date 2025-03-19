#!/bin/bash

if [ `mongo --eval "rs.isMaster()" | grep ismaster | grep true | wc -l` -eq 1 ]
then
    BASEDIR=/root/mongodump
    NAME=`date +%Y-%m-%d-%H-%M-%S`
    mkdir -p $BASEDIR/$NAME
    cd $BASEDIR/$NAME
    mongodump
    cd $BASEDIR
    tar cfpvJ `echo $NAME`.tar.xz `echo $NAME`
    rm -rv `echo $NAME`
    find $BASEDIR -ctime +7 | xargs rm -rv >> /root/mongodump.log
fi

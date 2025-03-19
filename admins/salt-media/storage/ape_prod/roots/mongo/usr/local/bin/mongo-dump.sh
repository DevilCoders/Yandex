#!/bin/bash

BASEDIR=/root/mongodump
NAME=`date +%Y-%m-%d-%H-%M-%S`
mkdir -p $BASEDIR/$NAME
cd $BASEDIR/$NAME
mongodump
cd $BASEDIR
tar cfpvJ `echo $NAME`.tar.xz `echo $NAME`
rm -rv `echo $NAME`
find $BASEDIR -ctime +4 | xargs rm -rv >> /root/mongodump.log
if [ `df -h | grep conta | awk '{print $(NF-1)}' | sed 's/%//g'` -gt 85 ]; then ls *xz | tail -n 2 | xargs rm -v; fi


#!/bin/bash
# MANAGED BY SALT
export PATH=/bin:/sbin:/usr/bin:/usr/sbin
dir=`dirname $BASH_SOURCE`
test -d /opt/mysql/cache/ || (cat $dir/schema.sql | mysql)

#!/bin/sh

set -xe

nm=$(hostname -f | sed -e 's/\./_/g' -e 's/\-/_/g')
echo "standby_mode = 'on'\nrecovery_target_timeline = 'latest'\nprimary_conninfo = 'host=$1 user=gpadmin password=123456 application_name=gp_walreceiver'" > $2

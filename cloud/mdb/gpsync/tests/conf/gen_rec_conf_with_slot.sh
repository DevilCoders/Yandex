#!/bin/sh
set -e

nm=$(hostname -f | sed -e 's/\./_/g' -e 's/\-/_/g')
echo "recovery_target_timeline = 'latest'\nprimary_conninfo = 'host=$1 application_name=$nm'\nprimary_slot_name = '$nm'" > $2
pgdata=$(pg_lsclusters | tail -n 1 | awk '{print $6}')
touch ${pgdata}/standby.signal

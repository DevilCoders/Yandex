#!/bin/bash
source /opt/greenplum-db-6/greenplum_path.sh
export MASTER_DATA_DIRECTORY=/var/lib/greenplum/data1/master/gpseg-1

cd ~gpadmin/gpconfigs
/opt/greenplum-db-6/bin/gpinitsystem -a --mirror-mode=group -c gpinitsystem_config -h segments -p gpopts.conf --su_password=123456 --ignore-warnings
/opt/greenplum-db-6/bin/gpstop -a

echo 'host all gpadmin 0.0.0.0/0 trust' >> $MASTER_DATA_DIRECTORY/pg_hba.conf
ssh gpsync-greenplum-master2.gpsync.gpsync.net "echo 'host all gpadmin 0.0.0.0/0 trust' >> $MASTER_DATA_DIRECTORY/pg_hba.conf"

#!/bin/bash

HOST=$(hostname)

pxf_cluster_status=$(timelimit -T 5 sudo -u gpadmin /opt/greenplum-pxf-5/bin/pxf cluster status 2>&1| egrep '(Could not connect to GPDB|PXF is not running on)')

[[ "${pxf_cluster_status}" =~ "Could not connect to GPDB" ]] && exit 0

pxf_failed_cnt=$(echo ${pxf_cluster_status} | sed -n 's/.*PXF is not running on \([0-9]*\).*/\1/p')

[ -z "$pxf_failed_cnt" ] && pxf_failed_cnt=0

echo "pxf,host=${HOST} pxf_failed_processes=${pxf_failed_cnt}i"

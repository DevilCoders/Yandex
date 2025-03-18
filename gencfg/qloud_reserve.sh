#!/usr/bin/env bash

base_path=`dirname "${BASH_SOURCE[0]}"`

cd $base_path || exit 1

./utils/hardware/qloud.py --segment qloud-ext.reserve > qloud_reserve_fqdn.txt
./dump_hostsdata.sh -s "#qloud_reserve_fqdn.txt" > qloud_reserve.txt

cat qloud_reserve.txt | ./utils/hardware/servers_order.py --hosts > qloud_reserve_stat.txt

echo qloud_reserve.txt
echo 
cat qloud_reserve_stat.txt

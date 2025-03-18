#!/usr/bin/env bash

base_path=`dirname "${BASH_SOURCE[0]}"`

$base_path/utils/hardware/preorders.py --full --preorder $@ > preorder_inv_full.txt
cat preorder_inv_full.txt | cut -f 2 > preorder_inv.txt

$base_path/dump_hostsdata.sh -s "#preorder_inv.txt" > preorder_hosts.txt &
$base_path/utils/common/show_machine_types.py -ns "#preorder_inv.txt" > preorder_groups.txt &

wait

echo preorder_inv.txt and preorder_inv_full.txt `cat preorder_inv_full.txt | wc -l`
echo
echo preorder_groups.txt
cat preorder_groups.txt | cut -f 1 -d :
echo
echo preorder_hosts.txt
cat preorder_hosts.txt | $base_path/sort.sh

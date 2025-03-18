#!/usr/bin/env bash

base_path=`dirname "${BASH_SOURCE[0]}"`

tmp_file=`mktemp`

$base_path/dump_hostsdata.sh -s "#$1" | $base_path/utils/hardware/fix_dump_hostsdata_output.py > $tmp_file

cat $tmp_file
echo
cat $tmp_file | $base_path/simple_evaluate.sh

rm $tmp_file

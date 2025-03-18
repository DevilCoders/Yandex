#!/usr/bin/env bash

base_path=`dirname "${BASH_SOURCE[0]}"`

cd $base_path
./gen.sh run_checks && ./utils/common/manipulate_hostsdata.py -a check && svn commit db "$@"

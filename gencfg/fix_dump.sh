#!/usr/bin/env bash

base_path=`dirname "${BASH_SOURCE[0]}"`

$base_path/utils/hardware/fix_dump_hostsdata_output.py "$@"

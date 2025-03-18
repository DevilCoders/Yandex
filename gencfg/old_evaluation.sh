#!/usr/bin/env bash

base_path=`dirname "${BASH_SOURCE[0]}"`

$base_path/utils/hardware/servers_order.py --hosts --mode yp 


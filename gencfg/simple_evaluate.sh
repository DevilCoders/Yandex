#!/usr/bin/env bash

base_path=`dirname "${BASH_SOURCE[0]}"`

$base_path/calculate_yp_quota_for_hosts/calculate_yp_quota_for_hosts --dump-hostsdata-input "$@"

#!/usr/bin/env bash

base_path=`dirname "${BASH_SOURCE[0]}"`

svn up $base_path $base_path/db

echo -e "\n"
$base_path/info.sh


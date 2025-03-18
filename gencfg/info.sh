#!/usr/bin/env bash

base_path=`dirname "${BASH_SOURCE[0]}"`

echo "Code: " && svn info $base_path | grep Rev && echo "" && svn st $base_path
echo -e "\nDb: " && svn info $base_path/db | grep Rev && echo "" && svn st $base_path/db


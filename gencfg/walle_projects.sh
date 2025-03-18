#!/usr/bin/env bash

base_path=$(dirname "${BASH_SOURCE[0]}")
tags_path=$base_path/walle_tags.txt

$base_path/utils/common/dump_hostsdata.py -i name,invnum,walle_tags "$@" >$tags_path
echo $tags_path
cat $tags_path | tr , \\n | grep g: | sort | uniq -c

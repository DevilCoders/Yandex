#!/bin/bash
cocaine-tool group list | grep -v prestable | grep -v echo | grep -v rbtorrent | grep -v ']' | grep -v '\[' | cut -d\" -f2 | while read name
do
    sleep 1
    group=`cocaine-tool group view -n $name | grep -v ":\ 0" | grep -v "{" | grep -v "}" | cut -d\" -f2`
    routing=`cocaine-tool routing -n $name | grep -v "{" | grep -v "}" | cut -d\" -f2`
    if [ $group != $routing ]
    then
        cocaine-tool group refresh -n $name
    fi
done

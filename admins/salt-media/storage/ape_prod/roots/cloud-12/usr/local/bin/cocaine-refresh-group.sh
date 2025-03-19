#!/bin/bash
cocaine-tool group list | cut -d\" -f 2 | grep v012 | while read name
do
    cocaine-tool routing -n $name
    if [ `echo $?` != 0 ]
    then
        cocaine-tool group refresh $name --timeout 60
        sleep 15
    fi
done


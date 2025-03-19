#!/bin/bash

2>/dev/null

federation_file='/etc/elliptics/federation'

if [ -e $federation_file ]
then
    federation=$(cat $federation_file)
else
    federation=1
fi

echo $@ a_federation_${federation}

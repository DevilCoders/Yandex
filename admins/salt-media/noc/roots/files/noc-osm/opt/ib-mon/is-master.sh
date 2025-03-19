#!/bin/bash

MASTER_LID=$( sudo /usr/sbin/sminfo | awk '/SMINFO_MASTER/{ print $4}' )
[ -n "$MASTER_LID" ] || exit 1

MASTER=$(sudo /usr/sbin/smpquery nodedesc $MASTER_LID | sed 's/.*\.\.//; s/ .*//')

[ "$MASTER" == `hostname -s` ]


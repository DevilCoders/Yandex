#!/bin/bash
if service mdb-backup-worker status 2>/dev/null | grep -q 'Active: active (running)'
then
    echo '0;OK'
else
    echo '2;mdb-backup-worker is not running'
fi

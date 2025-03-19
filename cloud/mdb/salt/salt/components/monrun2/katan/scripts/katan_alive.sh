#!/bin/bash
if /usr/bin/supervisorctl status mdb-katan 2>/dev/null | grep -q RUNNING
then
    echo '0;OK'
else
    echo '2;mdb-katan is not running'
fi

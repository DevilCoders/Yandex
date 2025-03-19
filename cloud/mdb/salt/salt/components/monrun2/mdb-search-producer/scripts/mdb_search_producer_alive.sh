#!/bin/bash
if /usr/bin/supervisorctl status mdb-search-producer 2>/dev/null | grep -q RUNNING
then
    echo '0;OK'
else
    echo '2;mdb-search-producer is not running'
fi

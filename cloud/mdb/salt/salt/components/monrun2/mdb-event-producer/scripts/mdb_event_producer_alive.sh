#!/bin/bash
if /usr/bin/supervisorctl status mdb-event-producer 2>/dev/null | grep -q RUNNING
then
    echo '0;OK'
else
    echo '2;mdb-event-producer is not running'
fi

#!/bin/bash

res=`psql postgres -t -A -X -c 'SELECT pg_is_in_recovery()'`
if [ $res == 'f' ]; then
    echo 'M'
else
    echo 'R'
fi

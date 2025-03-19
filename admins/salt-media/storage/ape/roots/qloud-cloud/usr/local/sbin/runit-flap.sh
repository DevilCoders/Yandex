#!/bin/bash

lock_file="/tmp/runit_flap_lock"
echo "2" > $lock_file

timetail -n 600 /tmp/runit_flap_log | cut -d] -f2 | sort | uniq -c | while read N name
do
    if [ $N -gt 4 ]
    then
        echo "`cat $lock_file`;$name" > $lock_file
    fi
done
if [ "`cat $lock_file`" != "2" ]
then
    echo `cat $lock_file` flap at last 10 minutes
else
    echo "0;Ok"
fi
rm $lock_file


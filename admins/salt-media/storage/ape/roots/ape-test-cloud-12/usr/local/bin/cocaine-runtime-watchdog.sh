#!/bin/bash

if [ `ubic status cocaine-runtime | grep off | wc -l` != 1 ]
then
    if [ `ubic status cocaine-runtime | grep -o "[0-9]*" | xargs lsof -p | wc -l` -gt 60000 ]
    then
	echo `date` too many open files `ubic status cocaine-runtime | grep -o "[0-9]*" | xargs lsof -p | wc -l` >> /var/log/cocaine-runtime-watchdog.log
        ubic restart cocaine-runtime
        exit 0
    fi
    cocaine-tool i --timeout 10 &> /dev/null
    if [ `echo $?` != 0 ]
    then
        cocaine-tool i --timeout 10 &> /dev/null
        if [ `echo $?` != 0 ]
        then
            cocaine-tool i --timeout 10 &> /dev/null
            if [ `echo $?` != 0 ]
            then
                if [ -f /var/log/cocaine-runtime-watchdog.log ]
                then
                    if [ `find /var/log/cocaine-runtime-watchdog.log -mmin -12 -print  | wc -l` != 1 ]
                    then
                        echo `date` `ubic status cocaine-runtime | grep -o [0-9]*` >> /var/log/cocaine-runtime-watchdog.log
                        echo `cocaine-tool i --timeout 5` >> /var/log/cocaine-runtime-watchdog.log
                        kill -s SIGABRT `ubic status cocaine-runtime | grep -o [0-9]*`
			sleep 20
                        ubic restart cocaine-runtime
                        exit 0
                    fi
                else
                    echo `date` `ubic status cocaine-runtime | grep -o [0-9]*` >> /var/log/cocaine-runtime-watchdog.log
                    echo `cocaine-tool i --timeout 5` >> /var/log/cocaine-runtime-watchdog.log
                    kill -s SIGABRT `ubic status cocaine-runtime | grep -o [0-9]*`
                    sleep 20
                    ubic restart cocaine-runtime
                    exit 0
                fi
            fi
        fi
    fi
fi


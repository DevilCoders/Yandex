#!/usr/bin/env bash

EXITCODE_MONGOS_WORKING=0
EXITCODE_STOPFLAG=3
EXITCODE_NO_MONGODB_CHECK_ALIVE=4
EXITCODE_MONGOS_RESTARTED=5
EXITCODE_MONGOS_FAILED_TO_RESTART=6

STOPFLAGFILE=/var/lib/mongos/watchdog.disable
MONGODB_CHECK_ALIVE=/usr/bin/mongodb-check-alive
MONGODB_SERVICE=mongos

if [ -e "${STOPFLAGFILE}" ] ; then
        exit "${EXITCODE_STOPFLAG}"
fi

if [ -e "${MONGODB_CHECK_ALIVE}" ] ; then
        check_result=`"${MONGODB_CHECK_ALIVE}" "${MONGODB_SERVICE}"`
        if [ "x${check_result}" = "x0;Ok;Ok" ] ; then
                exit "${EXITCODE_MONGOS_WORKING}"
        else
                service "${MONGODB_SERVICE}" restart >/dev/null 2>&1
                if [ "$?" -eq 0 ] ; then
                        exit "${EXITCODE_MONGOS_RESTARTED}"
                else
                        exit "${EXITCODE_MONGOS_FAILED_TO_RESTART}"
                fi
        fi
else
        exit "${EXITCODE_NO_MONGODB_CHECK_ALIVE}"
fi


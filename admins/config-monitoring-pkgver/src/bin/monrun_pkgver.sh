#!/bin/bash

# Sleep only under snaked job run.
# It usefull for `monrun -r pkgver` (including under executer's exec).
if [[ $JOB_NAME == "monrun_pkgver" ]]; then
    sleep $(($RANDOM % 60))
fi

if [[ -f "/etc/yandex-pkgver-ignore" ]]; then
        RES=$(/usr/bin/pkgver.pl | grep  -vf /etc/yandex-pkgver-ignore 2>/dev/null | wc -l);
        IGNORE_FILE=1
        IGNORE_COUNT=`wc -l /etc/yandex-pkgver-ignore`
        text_app=", and count of ignored packages: $IGNORE_COUNT"
else
        RES=$(/usr/bin/pkgver.pl | wc -l);
        IGNORE_FILE=0
        text_app=""
fi

if [ -z "$RES" ] || [ $RES -eq 0 ] ; then
        echo -n "0;ok"
        > /dev/shm/pkgverstate
else
        echo "diff" >> /dev/shm/pkgverstate
        STATE=$(cat /dev/shm/pkgverstate | wc -l);
                if [ $STATE -gt 10 ] ; then
                        echo -n "2;found $RES differences"
                elif [ $STATE -gt 2 ] ; then
                        echo -n "1;found $RES differences"
                else
                        echo -n "0;found $RES differences"
                fi
fi
echo $text_app

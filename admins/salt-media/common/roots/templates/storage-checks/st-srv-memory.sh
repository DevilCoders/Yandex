#!/bin/bash

if [ `uname -r |grep '2.6.32-4'` ]
then
    echo "0;Ok"
    exit 0;
fi

setlevel() {
    if [ ${level:-0} -lt $1 ]
    then
        level=$1
    fi
}

die() {
    IFS=';'
    echo "${level:-0};${msg[*]:-OK}"
    exit 0
}

usage() {
cat<<HELP
SYNOPSIS
  $0  [ OPTIONS ]
OPTIONS
  -u
      Example: ocraas:4G,tikaite:2G or ocraas:1024K or tikaite:2M

  -h
      Print a help message.

HELP
    exit 0
}

while getopts "u:h" OPTION
do
    case $OPTION in
        u)
            USERS="$OPTARG"
        ;;
        h)
            usage && exit 1
            ;;
        ?)
            usage && exit 1
            ;;
    esac
done

if [ ! -z "$USERS" ]
then
    IFS=',' read -ra U <<< "$USERS"
    for us in "${U[@]}"
    do
        user=`echo $us | awk -F ':' '{print $1}'`
        limit=`echo $us | awk -F ':' '{print $2}'`
        user_rss=`ps hax -o rss,user | awk -v u=$user '{if ($2== u) sum+=$1} END {print sum}'`
        
        if [ -z "$limit" ]
        then
            limit=1
        fi
        
        if [ -z "$user_rss" ]
        then
            user_rss=0
        fi
        
        if [[ $limit == *"G" ]]
        then
            limit=`echo $limit | sed 's/G//g'`
            limit=`echo $limit*1024*1024 | bc`
        elif [[ $limit == *"M" ]]
        then
            limit=`echo $limit | sed 's/M//g'`
            limit=`echo $limit*1024 | bc`
        elif [[ $limit == *"K" ]]
        then
            limit=`echo $limit | sed 's/K//g'`
        fi
        prc=`echo $user_rss/$limit*100 | bc -l 2>/dev/null| cut -f1 -d\.`
        if [ $(echo "$user_rss > $limit" |bc ) -eq 1 ]
        then
            setlevel 1; msg[m++]="$prc% of memory used $user"
        else
            msg[m++]="$prc% of memory used $user"
        fi
    
    done
else
        msg[m++]='Ok'
fi

die
exit 0

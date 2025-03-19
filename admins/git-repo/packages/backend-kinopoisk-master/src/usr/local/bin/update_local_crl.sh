#!/usr/bin/env bash

SRC_HOST="127.0.0.1"
SRC_FILE="/home/ca/rootCA/crl.crt"
DST_FILE="/etc/nginx/ssl_auth/crl.crt"
SERVICE="nginx"
                                                                                                                                                       
usage(){                                                                                                                                               
    echo "Usage: $0"                                                                                                                                   
    echo "It will update $DST_FILE from $SRC_HOST:$SRC_FILE"                                                                                           
    echo "and restart $SERVICE service"
}

ARGS=`getopt -o h --long help,debug -- "$@"`
eval set -- "$ARGS"

while true ; do
    case "$1" in
        -h| --help) usage; exit 0;;
        --debug) set -x; shift;;
        --) shift ; break ;;
        *) echo "Internal error!"; usage; exit 1;;
    esac
done

echo "update crl"
/usr/local/bin/crl_update.sh

remote_md5=$(ssh $SRC_HOST md5sum $SRC_FILE | awk '{print $1}')
local_md5=$(md5sum $DST_FILE | awk '{print $1}')

if [ "$remote_md5" != "$local_md5" ];then
        scp $SRC_HOST:$SRC_FILE $DST_FILE
        service $SERVICE restart 
fi
exit $?

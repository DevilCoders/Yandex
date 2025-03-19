#!/usr/bin/env bash
# Description: Send newly created satatic files to the backend.

usage(){
    echo "Usage:$0 <src_local_dir> <dst_remote_dir>"
    echo "    -h|--help Display this message."
    echo "    --debug   Run in debug mode."
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

if [ "${#@}" -lt 2 ];then 
    usage
    exit 1
fi

DST_HOST=graphdata
src_dir=$1
dst_dir=$2

ionice -c 3 rsync -a $src_dir $DST_HOST:$dst_dir

exit $?


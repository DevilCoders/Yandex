#!/bin/bash

HOST="$1"
DIR=$(dirname $(readlink -f $0))

scp -r \
    $DIR/parse_restore.py \
    $DIR/test_restore.sh \
    $DIR/test_restore.py \
    $DIR/tmux.sh \
    $DIR/yc_snapshot_client \
    $DIR/yc_nbs_client \
    $DIR/cloud \
    $DIR/grpc \
    $DIR/google \
    root@$HOST:~/

#ssh root@$HOST systemctl stop yc-snapshot
#scp $DIR/../yc-snapshot root@$HOST:/usr/bin
#ssh root@$HOST systemctl start yc-snapshot

#ssh -t root@$HOST ./tmux.sh
#ssh root@$HOST


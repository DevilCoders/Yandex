#! /usr/bin/env bash

var=`/usr/lib/etcd/etcdctl cluster-health | egrep unhealthy | wc -l`
if [[ $var == 0 ]]; then
    echo "0; ok"
else
    nodes=`/usr/lib/etcd/etcdctl cluster-health | egrep unhealthy | awk '{ print $2 }'`
    for node in $nodes; do
        name="`/usr/lib/etcd/etcdctl member list | egrep $node | sed -nre 's/(^name=|^.* name=)([^ ]+).*$/\2/; T; p'` $name"
    done
    echo "2; member ${name}down"
fi

#!/bin/bash
set -e
mkdir -p /var/lib/etcd/backup
crictl exec -ti $(crictl ps | grep etcd | cut -d\  -f1) etcdctl --endpoints='https://[::1]:2379' --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key --cacert=/etc/kubernetes/pki/etcd/ca.crt snapshot save /var/lib/etcd/backup/$(date "+%Y-%m-%d-%H-%M-%S").db
find /var/lib/etcd/backup/ -name "*db" -mtime +21 -type f -exec rm -rv {} \+

#!/bin/bash

set -ex

INSTANCES="kms-devel-hsm-vla-1.kms.cloud-preprod.yandex.net"

for INSTANCE in $INSTANCES; do
  pssh scp $INSTANCE-home.tar.gz $INSTANCE:/tmp/home.tar.gz
  pssh run "cd /; sudo tar xvf /tmp/home.tar.gz" $INSTANCE
done


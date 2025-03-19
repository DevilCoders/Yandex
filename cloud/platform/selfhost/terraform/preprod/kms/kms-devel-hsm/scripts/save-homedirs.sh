#!/bin/bash

set -ex

INSTANCES="kms-devel-hsm-vla-1.kms.cloud-preprod.yandex.net"

for INSTANCE in $INSTANCES; do
  pssh run "sudo tar cvzf /home.tar.gz /home" $INSTANCE
  pssh scp $INSTANCE:/home.tar.gz $INSTANCE-home.tar.gz
done


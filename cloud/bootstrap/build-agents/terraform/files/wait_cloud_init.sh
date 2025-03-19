#!/bin/bash
SLEEP_SEC=30

until test /var/lib/cloud/instance/boot-finished; do
  echo -e "\033[1;36mWaiting for cloud-init..."
  echo "wait ${SLEEP_SEC} sec";
  sleep ${SLEEP_SEC};
done
echo -e "\033[1;36mBoot finished!"

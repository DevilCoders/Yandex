#!/bin/bash

yc compute instance create --name win-kms-e2e-test-b \
                       --cores=2 \
                       --memory=2 \
                       --create-boot-disk type=network-ssd,image-id=fd8i7qi4jjhnb7qevtum \
                       --metadata-from-file user-data=/home/mrdracon/tools/scripts/windows-metadata.txt \
                       --platform-id standard-v2 \
                       --zone ru-central1-b \
                       --network-interface subnet-name=ipv4-b,nat-ip-version=ipv4 \
                       --network-interface subnet-name=cloudmkttestnets-ru-central1-b,ipv6-address=auto \
                       --folder-name mkt-test \
                       --service-account-name e2e-kms-sa

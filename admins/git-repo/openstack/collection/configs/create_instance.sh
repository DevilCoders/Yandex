#!/bin/bash

HOSTNAME=$1
IPv4=$2
IPv6=$3


cat ./configs/user-data-from-image.txt | sed "s/_HOSTNAME_/$1/g" > configs/user-data-from-image.$HOSTNAME.txt

PORT_ID=`neutron --os-region-name i port-create -f shell --variable id --fixed-ip  subnet_id=8a9bb506-db03-41e9-8ffc-679ef673de07,ip_address=$IPv4 --fixed-ip subnet_id=8dcd7cea-622d-4544-b5d5-90caa99491e9,ip_address=$IPv6 ca961ddb-e189-4b8a-87e5-20946abdc119 | cut -d '=' -f2 | tail -1 | tr -d '"'`

nova --os-region-name i boot --poll --flavor m1.large --image 3326e882-c428-42fb-aab7-a915f323cc7f --key-name azalio --nic port-id=$PORT_ID --user-data ./configs/user-data-from-image.$HOSTNAME.txt $HOSTNAME

rm ./configs/user-data-from-image.$HOSTNAME.txt

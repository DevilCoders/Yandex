#!/bin/bash

HOSTNAME=$1
#IPv4=$2
#IPv6=$3

#| 48dfebbe-07b4-44ed-8d60-4fe65f652bf5 | CORBANETS    | 7136c3ac-d186-4e51-a00d-54e4061eb899 5.255.228.128/25     |
#|                                      |              | d82b13a2-8bf8-4183-bc98-164bca6b31fd 2a02:6b8:0:1a5f::/64 

cat ./configs/user-data-from-image.txt | sed "s/_HOSTNAME_/$1/g" > configs/user-data-from-image.$HOSTNAME.txt

#--nic net-id=b4895c88-0b18-44d8-9762-3f4107bc0af1

#PORT_ID=`neutron --os-region-name i port-create -f shell --variable id --fixed-ip  subnet_id=8a9bb506-db03-41e9-8ffc-679ef673de07,ip_address=$IPv4 --fixed-ip subnet_id=8dcd7cea-622d-4544-b5d5-90caa99491e9,ip_address=$IPv6 ca961ddb-e189-4b8a-87e5-20946abdc119 | cut -d '=' -f2 | tail -1 | tr -d '"'`

#PORT_ID=`neutron port-create -f shell --variable id --nic net-id=48dfebbe-07b4-44ed-8d60-4fe65f652bf5 ca961ddb-e189-4b8a-87e5-20946abdc119 | cut -d '=' -f2 | tail -1 | tr -d '"'`

nova boot --poll --flavor m1.large --image 98ab33ba-ab99-44e5-b75c-86da6a50bbfe --key-name yozha --nic net-id=48dfebbe-07b4-44ed-8d60-4fe65f652bf5 --user-data ./configs/user-data-from-image.$HOSTNAME.txt $HOSTNAME.ocorba.yandex.net

rm ./configs/user-data-from-image.$HOSTNAME.txt

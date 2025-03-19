#!/bin/bash

if [ -z "$1" ] ;
  then
    echo "Usage: $0 conductor_group" ;
    exit 0 ;
fi
#Conductor Group
CG=$1
#Files
FH=/tmp/${CG}.hosts
> $FH;
FHI=/tmp/${CG}.hosts.ip
> $FHI
BFH=/tmp/balancers.${CG}
> $BFH
BFHU=/tmp/balancers.${CG}.uniq
> $BFHU

echo "hostlist:"
#executer hostlist %${CG} >> $FH 2>&1
curl -s http://c.yandex-team.ru/api/groups2hosts/$CG >> $FH
cat $FH ;
echo "ip of host"
for i in `cat $FH | grep yandex` ; do host $i | grep -vi ipv6| awk '{print $4}' | grep -v handled >> $FHI ; done
echo "ip6 of hosts"
for i in `cat $FH | grep yandex` ; do host $i | grep -i ipv6| awk '{print $5}' | grep -v handled >> $FHI ; done
echo "balancers of ip"
for i in `cat $FHI` ;  do curl -s https://racktables.yandex.net/export/slb-info.php?mode=vslist\&ip=${i} | grep -v maps >> $BFH ; sleep 1 ; done
#echo "uniq balancers"
cat $BFH | sort -u >> $BFHU;
cat $BFHU ;

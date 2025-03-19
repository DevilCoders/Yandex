#!/bin/bash
RES=`echo \`cat /proc/sys/net/netfilter/nf_conntrack_count\` \`cat /proc/sys/net/nf_conntrack_max\` | awk '{print int($1/$2*100)}'`
if [ $RES -lt 60 ]
then
    echo "0;OK"
else
    echo "2;nf_conntrack more then 60 percent"
fi

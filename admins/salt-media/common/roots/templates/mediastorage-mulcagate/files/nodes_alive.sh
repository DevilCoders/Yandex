#!/bin/bash

NODES=$1
WARN_PERC=3
CRIT_PERC=5

nodes=`curl -ss "localhost:11003/json/aggr" | grep '(?<="mulcagate-elliptics.nodes_alive.value_ahhh":\[\[)\d+(?=\])' -Po`

if [ -z $nodes ]
then
	echo "0;Ok"
	exit 0
fi

ratio=`echo "($NODES - $nodes) / $NODES * 100" | bc -l 2>/dev/null`
diff=`echo "$NODES - $nodes" | bc -l 2>/dev/null`

if [ $(echo "$ratio > $WARN_PERC" | bc -l 2>/dev/null) = 1 ]
then
	if [ $(echo "$ratio > $CRIT_PERC" | bc -l 2>/dev/null) = 1 ]
	then
        	echo "2;$diff nodes not found"
	else
        	echo "1;$diff nodes not found"
	fi
else
        echo "0;$nodes nodes alive"
	exit 0
fi

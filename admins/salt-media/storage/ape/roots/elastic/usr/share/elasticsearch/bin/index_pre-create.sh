#!/bin/bash

ES=localhost:9200
INDICES_PREFIXES="cocaine-v0.12-"
TS_FORMAT="%Y-%m-%d-%H"

NODE_ID=`curl -s -XGET "http://${ES}/_nodes/_local/settings" 2>/dev/null|sed -n 's/.*\"nodes\":{\"\([^\"]\+\)\":{\"name\":\".*/\1/ p'`
MASTER_ID=`curl -s -XGET "http://${ES}/_cluster/state/master_node?local=true" 2>/dev/null|sed -n 's/.*\"master_node\":\"\([^\"]\+\)\".*/\1/ p'`

if [ -n "${NODE_ID}${MASTER_ID}" ] ; then
	if [[ "$NODE_ID" == "$MASTER_ID" ]] ; then
		for p in $INDICES_PREFIXES ; do
			NEXT=$(( `date -u '+%s'` + 3600 ))
			INDEX=`date -u -d @${NEXT} "+${p}${TS_FORMAT}"`
			echo -e "`date` --- pre-creating index ${INDEX}"
			curl -s -XPUT "http://${ES}/${INDEX}"
			echo -e "\n`date` --- finished pre-creation of index ${INDEX}\n\n"
		done
	fi
fi

exit 0

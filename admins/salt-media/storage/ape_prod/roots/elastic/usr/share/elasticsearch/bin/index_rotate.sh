#!/bin/bash

: ${KEEP_DAYS:=0}
: ${KEEP_HOURS:=12}
: ${INDEX_PREFIX:=cocaine-v0.12-}

NODE_ID=`curl -s -XGET 'http://localhost:9200/_nodes/_local/settings' 2>/dev/null|sed -n 's/.*\"nodes\":{\"\([^\"]\+\)\":{\"name\":\".*/\1/ p'`
MASTER_ID=`curl -s -XGET 'http://localhost:9200/_cluster/state/master_node?local=true' 2>/dev/null|sed -n 's/.*\"master_node\":\"\([^\"]\+\)\".*/\1/ p'`

: ${KEEP_DAYS?Required parameter is not initialized} ${INDEX_PREFIX?Required parameter is not initialized}

if [ -n "${NODE_ID}${MASTER_ID}" ] ; then
	if [[ "$NODE_ID" == "$MASTER_ID" ]] ; then
		if [[ "$KEEP_DAYS" -lt 1 && "$KEEP_HOURS" -lt 1 ]] ; then
			echo -e "\n\tFATAL: Both KEEP_DAYS and KEEP_HOURS parameters has illegal values less than 1 at the same time.\n"
			exit 1
		else
			let CUT_UTS=$(date '+%s')-86400*$KEEP_DAYS-3600*$KEEP_HOURS
			for i in `curl -s -XGET localhost:9200/_cat/indices|awk "{if(\\$3~/^${INDEX_PREFIX}/){print \\$3}}"|sed -n "/^${INDEX_PREFIX}[[:digit:]]\{4\}[._-]\{1\}[[:digit:]]\{2\}[._-]\{1\}[[:digit:]]\{2\}\([._:-]\{1\}[[:digit:]]\{2\}\)\{0,1\}$/ p"|sort -n ` ; do
				CUR_UTS=$( date -d "`echo $i|sed -n -e 's/^[[:graph:]]\+\([[:digit:]]\{4\}[._-]\{1\}[[:digit:]]\{2\}[._-]\{1\}[[:digit:]]\{2\}\)\([._:-]\{1\}[[:digit:]]\{2\}\)\{0,1\}$/\1T\2/p'|sed -e 's/T$/T00/' -e 's/T[._:-]\{1\}\([[:digit:]]\{2\}\)$/T\1/' -e 's/\./-/g' -e 's/_/-/g'`:00:00,000000-0000" "+%s" )
				if [ "$CUR_UTS" -gt 1 ] ; then
					if [ "$CUR_UTS" -lt "$CUT_UTS" ] ; then
						echo removing index: $i
						curl -s -XDELETE "localhost:9200/$i"
						echo -e "\nfinished.\n"
					fi
				fi
			done
		fi
	fi
fi

exit 0

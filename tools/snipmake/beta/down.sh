#!/usr/local/bin/bash

set -e
set -o pipefail

ME=mark4
BS=httpsearch_$ME
SEARCHES="-i testws-production-replica"
MY_PATH=/var/tmp/sky_snippets_$ME

if [ ! -f hosts ] 
then
    ./query_instances $SEARCHES --hosts hosts
fi

echo "Down: start"
sky run --hosts hosts "cd $MY_PATH; ./kill.sh $BS; rm -Rf $MY_PATH"
echo "Down: done"


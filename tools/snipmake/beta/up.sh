#!/usr/local/bin/bash

set -e
set -o pipefail

ME=mark4
BS=httpsearch_$ME
BP=8909
CF=$ME.cfg
MY_PATH=/var/tmp/sky_snippets_$ME
TB=$ME.tar.gz
MESSAGE="too lazy to write anything"
SHARD_TAG_OPTS=""
SHARD_TAGS="-s RusTier0"
INSTANCE="testws-production-replica"

while getopts "m:s:i:" opt ; do
    case $opt in
        m ) MESSAGE="$OPTARG";;
        s ) SHARD_TAG_OPTS="$SHARD_TAG_OPTS -s $OPTARG";
            SHARD_TAGS="$SHARD_TAG_OPTS";;
        i ) INSTANCE="-i $OPTARG";;
    esac
done

echo "Shards info..."
./query_instances -i $INSTANCE $SHARD_TAGS --manifest $BS.manifest --hosts hosts --shards shards -m "$MESSAGE"

echo "Create dir..."
set +e
sky run --hosts hosts "mkdir $MY_PATH"
set -e 

echo "Create tarball..."
tar -czf $TB $BS $CF $BS.manifest shards kill.sh run.sh

echo "Upload tarball..."
set +e
#TODO: sky upload --hosts=hosts is not supported for some reason
sky upload $TB $MY_PATH `awk '{print "+"$1}' hosts`
set -e

echo "Up: start"
set +e
sky run --hosts hosts "cd $MY_PATH && tar -xf $TB && chmod +x run.sh kill.sh $BS && ./run.sh $BS $BP $CF"
set -e

echo "Up: success"


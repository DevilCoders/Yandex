#!/usr/local/bin/bash

set -e
set -o pipefail

ME=mark4
HOST=scrooge.yandex.ru

PATH=$PATH:/Berkanavt/bin/scripts
MS=middlesearch_$ME
CFG=$MS.cfg
MANIFEST=$MS.manifest
TB=$MS"_tar".tar.gz
MY_PATH=/var/tmp/sky_snippets_$ME
SHARD_TAG_OPTS=""
SHARD_TAGS="-s RusTier0"
INSTANCE="testws-production-replica"

while getopts "c:m:s:i:" opt ; do
    case $opt in
        c ) CUSTOM_CONFIG="$OPTARG";;
        m ) MESSAGE="$OPTARG";;
        s ) SHARD_TAG_OPTS="$SHARD_TAG_OPTS -s $OPTARG";
            SHARD_TAGS="$SHARD_TAG_OPTS";;
        i ) INSTANCE="-i $OPTARG";;
    esac
done

echo "---------------------------------------------"
if [ -n "$CUSTOM_CONFIG" ]; then
    echo "Using configuration file: $CUSTOM_CONFIG"
    ./query_instances -m "$MESSAGE" --manifest $MANIFEST --program $MS
    cp "$CUSTOM_CONFIG" $CFG
else
    echo -n "Generating config..."
    ./query_instances -m "$MESSAGE" $INSTANCE $SHARD_TAGS --shards middle_shards --manifest $MANIFEST --program $MS
    cut -f 1 middle_shards | python gen_middle.py > $CFG
    echo "done"
fi

echo "---------------------------------------------"
echo -n "Creating archive..."
tar -cLf $TB $MS $CFG $MANIFEST kill.sh

if [ ! -f $TB ]
then
    echo "ERROR: can't create archive $TB"
    return 1
fi
echo "done"

echo "---------------------------------------------"
echo -n "sky: Creating dir..."
sky run "mkdir $MY_PATH" +$HOST
echo "done"

echo -n "sky: Uploading archive..."
sky upload $TB $MY_PATH +$HOST
echo "done"

echo -n "sky: Unpacking archive..."
sky run "cd $MY_PATH && sleep 2 && tar -xf $TB*" +$HOST
echo "done"

echo -n "sky: Starting middlesearch..."
sky run "cd $MY_PATH; daemon -f -p $MS.pid ./$MS -d $CFG" +$HOST
echo "done"

##Local start
#echo "Starting middlesearch"
#nohup ./$MS -d $CFG &
#echo "Done"

echo "---------------------------------------------"
echo "Testing middlesearch..."
set +e
fetch -o test_$ME.html http://$ME.yandex.ru/yandsearch?text=test\&sbh=1 </dev/null #>/dev/null 2>/dev/null </dev/null
set -e
if ! grep -q $HOST test_$ME.html
then
    echo "ERROR: it is not alive..."
    exit 1
fi
rm -f test_$ME.html
echo "Success"


#!/usr/bin/env bash
set -e -o pipefail

KWHOST=scrooge.search.yandex.net
KWPORT=32068

URL_LIST=urls.txt
QUERY=PrewalrusExport.query

BINDIR=./bin
DATADIR=./data
ATTRDIR=$DATADIR/attrs
INDEXDIR=$DATADIR/newindex
WALRUSDIR=$DATADIR/fakeprewalrus_out
BSPID=$DATADIR/basesearch.pid

BSPORT=37193
BSCONF=$DATADIR/yandex.cfg

set +e
killall -q $BINDIR/basesearch
set -e

echo "Reading the Prewalrus export from KiWi at $KWHOST:$KWPORT"
rm -f $DATADIR/kwworm.out
set +e
pv -l $URL_LIST | $BINDIR/kwworm -c $KWHOST --port $KWPORT --maxrps 100 read -d 100 -f protobin -s -k 50 -Q $QUERY > $DATADIR/kwworm.out
set -e

echo "Filtering KiWi records to remove empty responses"
$BINDIR/prewalrusfilter < $DATADIR/kwworm.out > $DATADIR/reindexed_out/1.out

echo 'Building search index with fakeprewalrus'
$BINDIR/fakeprewalrus -o $WALRUSDIR/%03d-index -p keyinv -n 1 -h $DATADIR -t $WALRUSDIR/temp $DATADIR/reindexed_out --keyinv_max_sub_portions 30 -f protobin
$BINDIR/fakeprewalrus -o $WALRUSDIR/%03d-index -p arcdir -n 1 -h $DATADIR -t $WALRUSDIR/temp $DATADIR/reindexed_out --keyinv_max_sub_portions 30 -f protobin
$BINDIR/fakeprewalrus -o $WALRUSDIR/%03d-index -p tagtdr -n 1 -h $DATADIR -t $WALRUSDIR/temp $DATADIR/reindexed_out --keyinv_max_sub_portions 30 -f protobin

mkdir -p $ATTRDIR
mkdir -p $INDEXDIR

for f in key inv arc dir tag tdr; do    
    cp $WALRUSDIR/000-index$f $INDEXDIR/index$f
done

echo 'Building indexaa and url.dat files'
$BINDIR/tar_to_urldat $WALRUSDIR/000-indextag $INDEXDIR $ATTRDIR
echo -e "$ATTRDIR/d.d2c\n$ATTRDIR/h.d2c" > $ATTRDIR/grattr.conf
$BINDIR/grattrgen -c $ATTRDIR/grattr.conf -o $INDEXDIR/indexaa

echo 'Spawning the basesearch daemon'
if ! $BINDIR/basesearch -VINDEX=$INDEXDIR -VPORT=$BSPORT -VPID=$BSPID $BSCONF; then
    echo Basesearch returned error code, ignoring
fi
sleep 5

echo 'Generating snippet contexts'
cat $URL_LIST | sed 's|http://||' | awk '{print "url:"$0}' | $BINDIR/hamsterwheel -m localhost:$BSPORT > contexts.txt

echo 'Banishing the basesearch daemon'
killall $BINDIR/basesearch

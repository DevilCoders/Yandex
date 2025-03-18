#!/usr/bin/env bash
set -e

testname=$1
arcadia_dir=$2
bin_dir=$3
work_dir=$4
tests_dir=$arcadia_dir/tools/clustermaster/tests

# preinit
export PATH=$tests_dir/bin:$PATH
export HOSTNAME=twalrus4
export VBOX_ROOT=$work_dir/$(mktemp -u)
export VBOX_HOSTS=$tests_dir/hosts.lst

# common
mkdir -p $VBOX_ROOT/$HOSTNAME/Berkanavt/RobotRegressionTest/Data
echo -n 8001 > $VBOX_ROOT/$HOSTNAME/Berkanavt/RobotRegressionTest/Data/spider_port.txt
ln -sf $arcadia_dir/check/robotcheck $VBOX_ROOT/$HOSTNAME/Berkanavt/RobotRegressionTest/
ln -sf $bin_dir $VBOX_ROOT/$HOSTNAME/Berkanavt/bin
MSG_SERVER_PORT=22222
echo -n $MSG_SERVER_PORT > $VBOX_ROOT/$HOSTNAME/message_server.port
mkdir -p $VBOX_ROOT/$HOSTNAME/$testname/CLUSTER_MASTER
mkdir -p $VBOX_ROOT/$HOSTNAME/tmp

> $VBOX_ROOT/vboxhosts.map

for h in cmx500 cmx501 cmx502 cmx503 cmx504 cmx505 cmx506 cmx507 cmx508 cmx509 cmx510 cmx511 cmx512 cmx513 cmx514 cmx515; do
    cp -rf $VBOX_ROOT/twalrus4 $VBOX_ROOT/$h
done

# in jail
robotcheck=/Berkanavt/RobotRegressionTest/robotcheck
datdir=/
bindir=/Berkanavt/bin
tests=$robotcheck/tests
checkdir="DUMMY"

target="run"

ln -sf $arcadia_dir/check/robotcheck/tests/config/cm/linux/$testname $VBOX_ROOT/$HOSTNAME/$testname/config

exec 2> $VBOX_ROOT/errout
set -x
$VBOX_ROOT/$HOSTNAME/$robotcheck/message_server.py $MSG_SERVER_PORT &
MSG_SERVER_PID=$!

function cleanup_on_exit() {
    set +e
    kill $(cat $VBOX_ROOT/$HOSTNAME/$testname/CLUSTER_MASTER/master_pid)
    kill $(cat $VBOX_ROOT/$HOSTNAME/$testname/CLUSTER_MASTER/worker_pid)
    kill $(cat $VBOX_ROOT/$HOSTNAME/$testname/CLUSTER_MASTER/solver_pid)
    kill $MSG_SERVER_PID
    rm -rf $VBOX_ROOT
    #exit 0
}
trap "cleanup_on_exit; exit 1" INT TERM ERR
trap "cleanup_on_exit; exit 0" EXIT

LD_PRELOAD=$bin_dir/libvbox.so TMPDIR=$VBOX_ROOT/$HOSTNAME/tmp \
$VBOX_ROOT/$HOSTNAME/$robotcheck/run_cm_test.sh \
    --test $testname \
    --tests_dir $tests \
    --robotcheck $robotcheck \
    --bindir $bindir \
        --spider_nocache_bindir "DUMMY" \
    --datdir "$datdir/$testname" \
    --checkdir $checkdir \
    --logdir $datdir \
        --version "DUMMY" \
    --cmdir "$datdir/$testname/CLUSTER_MASTER" \
    --rundir $datdir \
    --target $target \
        --run-rev "DUMMY" \
        --run-branch "DUMMY" 

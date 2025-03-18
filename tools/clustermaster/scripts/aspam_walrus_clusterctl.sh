#!/bin/sh -e

if [ `whoami` != "aspam" ] ; then
	echo "Run this script from aspam user!" >&2
	exit 1
fi

do_kill=
do_run=
do_put=
master_only=
workers_only=
workers_auto=

auth=
var=
hostconf=
customworkers=

extraworkers=
put_extraworkers=

script=
urlprefix=
master_http_port=
master_pidfile=
ro_urlprefix=
ro_master_http_port=

worker_port=
worker_http_port=
worker_pidfile=

phconfig='phconfig -c /Berkanavt/database/host.cfg'
worker_binary='/Berkanavt/antispam/cm/bin/worker'

die() {
    echo "$@"
    exit 1
}

usage() {
    echo "Usage: $0 [-krtmw] [-l logfile] -a auth_key -s script"
    echo
    echo "Action"
    echo "       -k   kill cluster"
    echo "       -r   run cluster (may be specified along with -k to restart after kill)"
    echo "       -t   put worker to hosts"
    echo "       -m   work with master only"
    echo "       -w   work with workers only"
    echo "       -A   for -r, get list of inactive workers from master"
    echo
    echo "Common params"
    echo "       -a   path to auth key"
    echo "       -v   path to vardir"
    echo "       -c   path to hostconf"
    echo "       -C   path to custom host file"
    echo
    echo "       -e   hostnames of extra [global] workers, space separated"
    echo "       -E   hostnames of extra workers to put worker binary to, space separated"
    echo
    echo "Master params"
    echo "       -s   path to script"
    echo "       -u   http url prefix "
    echo "       -h   master http port"
    echo "       -p   pidfile"
    echo "       -U   read-only http url prefix "
    echo "       -R   read-only master http port"
    echo
    echo "Worker params"
    echo "       -W   worker port"
    echo "       -H   worker http port"
    echo "       -P   pidfile"
    exit 1
}

do_run() {
    echo "===> Starting cluster (ports $master_http_port $worker_port $worker_http_port)"
    if [ -z "$workers_only" ] ; then
        echo "[*] Running master..."
        cd /Berkanavt/antispam/cm && ./bin/master $common_args $master_args
    fi

    [ "$master_only" ] && return
    
    run_list="$extraworkers `$phconfig h W` `yr +ZM LIST | sort`"
    if [ -n "$workers_auto" ] ; then
        run_list=`fetch -qo /dev/stdout http://localhost:$master_http_port/workers_text | awk '$2 != "active"{print $1}'`
    fi

    echo "[*] Running workers..."
    for h in $run_list; do
        rsh -n $h "/usr/local/etc/rc.d/clustermaster workerstart antispam >/dev/null" &&
          echo -n . &
        sleep 0.05
    done
    wait
    echo

    echo "[*] Done!"
}

do_kill() {
    echo "===> Killing cluster (ports $master_http_port $worker_port $worker_http_port)"
    if [ -z "$workers_only" ] ; then
        echo "[*] Killing master..."
        test -e $master_pidfile && cat $master_pidfile | xargs kill
    fi

    [ "$master_only" ] && return

    echo "[*] Killing workers..."
    for h in $extraworkers `$phconfig h W` `yr +ZM LIST | sort`; do
        rsh -n $h "test -e $worker_pidfile && cat $worker_pidfile | xargs kill" &
        echo -n .
        sleep 0.02
    done
    wait
    echo

    echo "[*] Done!"
}

do_put() {
    echo "===> Updating worker binary"

    date=`stat -f%Sm -t%s $worker_binary`

    for h in $put_extraworkers; do
        rcp -p $worker_binary $h:/Berkanavt/antispam/cm/bin/worker.new &&
        rsh -n $h "cd /Berkanavt/antispam/cm/bin; mv -f worker.new worker; echo $h"
    done

    echo "(copying to walrus000)"
    rsync -t $worker_binary walrus000:/Berkanavt/antispam/cm/bin/worker.$date
    rsh -n walrus000 'set -e
        cd /Berkanavt/antispam/cm/bin
        for h in `phconfig h W`; do
            rcp -p worker.'$date' $h:/Berkanavt/antispam/cm/bin/worker.new &&
            rsh -n $h "cd /Berkanavt/antispam/cm/bin; mv -f worker.new worker; echo $h" &
            sleep 0.2
        done
        wait; rm worker.'$date'
    '

    echo "(copying to +ZM)"
    for h in `yr +ZM LIST | sort`; do
      rsync -pt $worker_binary $h:/Berkanavt/antispam/cm/bin/worker && echo $h &
      sleep 0.4
    done
    wait

    echo "[*] Done!"
}

while getopts krmtwAa:v:c:C:e:E:s:u:h:p:W:H:P:U:R: opt; do
    case "$opt" in
    k)	do_kill=yes;;
    r)	do_run=yes;;
    t)	do_put=yes;;
    m)	master_only=yes;;
    w)  workers_only=yes;;
    A)  workers_auto=yes;;

    a)	auth="$OPTARG";;
    v)	var="$OPTARG";;
    c)	hostconf="$OPTARG";;
    C)	customworkers="$OPTARG";;

    e)	extraworkers="$OPTARG";;
    E)	put_extraworkers="$OPTARG";;

    s)	script="$OPTARG";;
    u)	urlprefix="$OPTARG";;
    h)	master_http_port="$OPTARG";;
    p)	master_pidfile="$OPTARG";;
    U)	ro_urlprefix="$OPTARG";;
    R)	ro_master_http_port="$OPTARG";;

    W)	worker_port="$OPTARG";;
    H)	worker_http_port="$OPTARG";;
    P)	worker_pidfile="$OPTARG";;
    ?)	usage;
    esac
done

if ! [ "$do_run" -o "$do_kill" -o "$do_put" ]; then
    usage
fi

# sanity
[ -z "$hostconf" -a -z "$customworkers" ] && die "-c or -C required"
[ -z "$var" ] && die "-v required"
[ -z "$script" ] && die "-s required"
[ -z "$master_pidfile" ] && die "-p required"
[ -z "$worker_pidfile" ] && die "-P required"

# defaults
[ -z "$master_http_port" ] && master_http_port=3130
[ -z "$worker_port" ] && worker_port=3131
[ -z "$worker_http_port" ] && worker_http_port=3132

# common
common_args="-v $var"
[ -n "$auth" ] && common_args="$common_args -a $auth"

# master
master_args="-s $script -P $master_pidfile -h $master_http_port -w $worker_port"
[ -n "$urlprefix" ] && master_args="$master_args -u $urlprefix"
[ -n "$hostconf" ] && master_args="$master_args -c $hostconf"
[ -n "$customworkers" ] && master_args="$master_args -C $customworkers"
[ -n "$ro_urlprefix" ] && master_args="$master_args -U $ro_urlprefix"
[ -n "$ro_master_http_port" ] && master_args="$master_args -H $ro_master_http_port"

# worker
worker_args="-P $worker_pidfile -h $worker_http_port -w $worker_port"

# temp hack untill new host.cfg
master_args="$master_args -c /Berkanavt/database/host.cfg"

# extra
[ "$do_put" ] && do_put
[ "$do_kill" ] && do_kill
[ "$do_run" ] && do_run

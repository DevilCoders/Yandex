#!/bin/bash

MONGO_BACKUP_ENABLED=0
R_PASSWORD='fvgbhnjm'
R_REALM='mongo-backup'
R_SERVER='localhost'
R_USER='maps'
STANDALONE=0
IS_HIDDEN=1 # Hidden slave
SHARD_NAME=''
MONGOS_HOST=''
MONGO_MONGOS_PORT=27017
ZK_FLOCK_CONFIG='/etc/distributed-flock-mongo-backup.json'

MONGO_DUMP_PORT=27018
MONITORING_FILE='/var/lib/mongo-backup/backup.rslt'
MONGO_DATA_DIR='/var/lib/'
LOG_FILE="/var/log/mongo-backup.log"
MONGOD_VERSION=0   # 0 - unknown

# always silently include
. /etc/mongo-backup.conf &>/dev/null

# XXX: this vars not overwriteable from config
MODE_SARDED=0 # Mongos over replica-set
MODE_SINGLE=1 # The lonely server no locks and state checks need since we standalone
MODE_REPLICA=2  # Standalone replica-set

# XXX: pass all output to log file, don't move and don't change!!!
init_logging() {
    if [ "$LOG_FILE" ];then
        # rotate logs for every backup in separate file
        for i in {14..1}; do
            local lnum=".$((i-1))" ; # from 0 but 0 will be strip on next line
            mv $LOG_FILE${lnum#.0} $LOG_FILE.${i}|&: # always success
        done
        exec 3<>$LOG_FILE; exec 1>&3; exec 2>&1
    fi
}
init_logging

# setting up mongos and mongodb authentication arguments
# no auth required, don't pass auth-related args
MONGOS_AUTHARGS="${M_USER:+--authenticationDatabase admin -u "$M_USER" -p "$M_PASSWD"}"
MONGODB_AUTHARGS="${M_USER:+--authenticationDatabase admin -u "$M_USER" -p "$M_PASSWD"}"
COMPRESSION="${COMPRESSION:-pigz}"

# Fancy logging
function log {
    # ret = save return code from cmd runned in log message
    # like log "$(/sbin/initctl start mongodb)" in this case
    # log return status code from `/sbin/initctl start mongodb`
    local ret=$? ts="$(date '+%FT%T')" lineno=`caller 0`
    : ${lineno:="- $0 x"}
    printf -- "${ts//%/}:[${$//%/}] %4s#%-15s: ${@//%/%%}\n" ${lineno% *}
    return $ret
}; export -f log

function monitoring {
    local status="$1" message="$2"

    if ((status == -1));then
        truncate -s0 "$MONITORING_FILE"
        status=0
    fi
    log "$message"
    if [[ $status =~ ^[012]$ ]]; then
        echo "$(date +%s) $status; $message" >> "$MONITORING_FILE"
    else
        echo "$(date +%s) 2; Invalid status with '$message'">> "$MONITORING_FILE"
    fi
}

function conductor {
   # conductor <method> <conductor_entity(host,group i.e)> format
   local api="http://c.yandex-team.ru/api-cached" _r=0
   until curl -sf "$api/$1/$2${3:+?format=}$3"; do
      if ((_r+=1, _r > ${MAX_CURL_RETRIES:=3})); then
         log "Failed to curl conductor #${MAX_CURL_RETRIES} times, exit!"
         exit 1
      fi
   done
}


function check_mongodb_state {
    local host="localhost:$MONGO_DUMP_PORT"
    log "Checking RS member status"
    my_state=$(mongo --quiet $host $MONGODB_AUTHARGS <<<'rs.status().myState' 2>/dev/null)
    if [[ $my_state != '2' && $BREAK_ON_WRONG_STATE == 1 ]]; then
        log "Error, state is $my_state, skipping backup"
        monitoring 1 "Skipping backup due to wrong RS member state"
        exit 1
    else
        log "OK, state is $my_state, proceeding with backup"
    fi
}


function set_oplog_counter {
    log "Setting oplog pointer"
    CUR_POS=$(echo "var stat=rs.status();
    stat.members.forEach(function(x){
        if(x.self){print('Timestamp('+x.optime.t+', '+x.optime.i+')')}
    });" | mongo --ipv6  localhost:$MONGO_DUMP_PORT $MONGODB_AUTHARGS --quiet | grep -v 'bye')
    echo "$CUR_POS" > /var/tmp/mongo-oplog-cursor
    log "Set oplog pointer to $CUR_POS"
}


function resolve_mongos_host {
    # if mongos host not set, try resove one from mongos group
    # if mongos group not set exit with error!
    local hosts=""
    if test -n "$MONGOS_GROUP"; then
        hosts=`conductor groups2hosts $MONGOS_GROUP` || exit 1
    fi
    for host in $MONGOS_HOST $hosts; do
        mongo --ipv6 $host:$MONGO_MONGOS_PORT $MONGOS_AUTHARGS --eval "sh._checkMongos()"
        if (($? == 0)); then
            MONGOS_HOST="$host"
            break
        fi
    done
    log "Mongos is $MONGOS_HOST"
}


# Lock mongos balancer. We don't want chunk moving during the backup.
function lock_config {
    log "Locking balancer."
    log "Connecting to $MONGOS_HOST:$MONGO_MONGOS_PORT"
    local query="db.getSisterDB('config').settings.update(
            {_id: 'balancer'},{\$set: {stopped: true}},true);"
    log "$(mongo --ipv6 $MONGOS_HOST:$MONGO_MONGOS_PORT $MONGOS_AUTHARGS <<<"$query" 2>&1)"
	if (($? != 0 )); then
        monitoring 2 "Failed to lock mongo balancer!" >&2
        exit -1
    fi
    log "Wait balancer locking."
    local loop='
        var timeout=3600
        while(sh.isBalancerRunning() || timeout < 0) {
            print("Waiting... 1m");
            sleep(60*1000);
            timeout -= 60;
        };
        if (timeout < 0) {
            print("Lock balancer timeout! Check config servers state")
        }
    '
    coproc {
        mongo --ipv6 $MONGOS_HOST:$MONGO_MONGOS_PORT $MONGOS_AUTHARGS <<<"$loop"
    }
    while read -u ${COPROC[0]} &>/dev/null; do
        log "Balancer lock log: $REPLY"
    done
    wait $COPROC_PID
    ret=$?
    if (($ret != 0)); then
        log "Failed to lock balancer"
        if test -n "$IGNORE_BALANCER_LOCK_FAILED"; then
            log "Exit by not empty var IGNORE_BALANCER_LOCK_FAILED=$IGNORE_BALANCER_LOCK_FAILED"
            exit 1
        fi
    fi
}

# Unlock balancer
function unlock_config {
    local host=${MONGOS_HOST}:${MONGO_MONGOS_PORT}
    log "Waiting for all shards to be dumped."
    local query='
        db.getSisterDB("admin").runCommand({listshards: 1}).shards.forEach(
            function(x) {
                print(x._id)
            }
        );
        '
    local slist=$(
        mongo --ipv6 $host ${MONGOS_AUTHARGS} --quiet <<<"$query"|\
        egrep -v '^bye$'
    )
    log "Got list of shards:\n$slist"
    while true; do
        for i in $slist; do
            # 82 is reserved exit code,  # do not remove this line see Makefile about known-codes
            # if zk-flock return 82 then zk lock is locked
            zk-flock -w 10 -x 82 -c $ZK_FLOCK_CONFIG "$i-dump-lock" "bash -c 'echo Shard $i Unlocked'"
            if (($? == 82)); then
                log "Shard $i locked, wait..."
                sleep 1m
                continue 2
            fi
        done
        break
    done

    if [ "${DISABLE_BALANCER:-0}" -ne 0 ] ; then
        local state="true" msg= "Keeping balancer stopped."
    else
        local state="false" msg="Unlocking balancer."
    fi
    query="db.getSisterDB('config').settings.update(
           {_id: 'balancer'},{\$set: {stopped: $state}},true);"

    if ! mongo --ipv6 $host $MONGOS_AUTHARGS <<<"$query" &> /dev/null; then
	  monitoring 1 "Failed to $msg" >&2
    else
      log "Succesfully $msg"
    fi
}

# Dump current shard from local files
function dump_shard {
    local dumpdir=$1 suffix="gz"

    if /sbin/initctl status mongodb|&grep -q running; then
        # prepare database only if mongod running
        log "Preparing database"
        local cmd="db.getSisterDB('admin').fsyncLock();"
        log "Run mongo command: $cmd"
        log "$(mongo localhost:$MONGO_DUMP_PORT $MONGODB_AUTHARGS --quiet <<<"$cmd" 2>&1)"
        if (($? != 0)); then
            monitoring 2 "Failed to fsync data." >&2
            exit 5
        fi
    fi

    # DO NOT move stopping mongodb from this function
    # STOP SHOULD PERFORMED ONLY UNDER ZK LOCK
    if ((MONGOD_VERSION < 320)); then
        log "old mongod, fsyncLock don't give data consistency without stopping"
        log "Stopping mongodb: $(/sbin/initctl stop mongodb 2>&1)"
        sleep 5
    else
        log "New mongod (> 3.2.0), fsyncLock give data consistency without stopping"
    fi

    if [[ "$COMPRESSION" =~ zst ]]; then
        local suffix="zst"
    fi
    log "Dumping shard. Change working directory to $MONGO_DATA_DIR"
    cd  $MONGO_DATA_DIR

    local tarfile="$dumpdir/shard.dump.$(date +%Y.%m.%d).tar.$suffix"
    log "Tar mongodb directory to $tarfile"
    sync; sync
    tar --exclude mongodb/diagnostic.data -I "$COMPRESSION" -cf $tarfile mongodb &
    local tar_pid=$!
    while kill -0 $tar_pid 2>/dev/null; do
        sleep 1
        if ((__tar_pid_sleep_counter++ % 60 == 0)); then
            log "Progress... Dump size $(du -sh $dumpdir)"
        fi
    done
    wait $tar_pid
    local ret=$?
    if ((ret != 0)); then
        if ((MONGOD_VERSION >= 320)); then
            local free_space=$(df -m $dumpdir|awk 'NR==2{print $4}')
            if ((free_space > 100)); then # free > 100 mb
                log "Ignore tar status on mongo > 3.2.0, dump dir contains free space $free_space mb"
                ret=0
            fi
        else
            monitoring 2 "Failed to tar shard, ret code: $ret" >&2
        fi
    else
        log "Tar complete successfully. Dump size $(du -sh $dumpdir)"
        # Need additional sleep to prevent quick lock release
    fi
    log "Result Dump size $(du -sh $dumpdir)"
    if ((ret == 0)); then
        log "Sleep 60 seconds to prevent backups on other slaves"
        sleep 60
    fi
    return $ret
}

# Dumping config db and users through mongodump
function dump_to_bson {
    local target="$1" db="$2" col="$3"  # col - abbreviation of collection
    local args="$MONGODUMP_ARGS $MONGOS_AUTHARGS --db $db"

    local out="--out $target"
    if mongodump --help|&fgrep -qe '--archive'; then
        out="--gzip --archive=${target}/$db.dump.gz"
    fi
    local msg="Dumping '$db${col:+".$col"}' database to '$out'"
    log "$msg"

    if ! [[ "$args" =~ --ipv6 ]] && mongodump --help|&fgrep -qe '--ipv6'; then
        args="$args --ipv6 "
    fi

    log "$(mongodump -h $MONGOS_HOST $args ${col:+ --collection $col} $out)"
    if [[ $? -ne 0 ]]; then
        monitoring 2 "Failed to dump config db." >&2
    else
        log "Succesfully $msg"
    fi
}

function downtime_host {
    OK=0
    local opts="do=1&object_name=$(hostname)&end_time=%2B${DT_HOURS}hours&description=${DT_DESCRIPTION}"
    if [[ $USE_DOWNTIME -ne 0 ]]; then
        for host in ${JCTL_HOSTS[*]}; do
            log "Trying to downtime for $DT_HOURS hours at $host"
            curl -ks "http://${host}:8998/api/downtimes/set_downtime?$opts"
            OK=$?
            if [[ ${OK} == 0 ]]; then
                break
            fi
        done
    fi
    return $OK
}

function remove_downtime {
    OK=0
    if [[ $USE_DOWNTIME -ne 0 ]]; then
        for host in ${JCTL_HOSTS[*]}; do
            log "Trying to remove downtime from host at $host"
            curl -ks "http://${host}:8998/api/downtimes/remove_downtime?do=1&object_name=$(hostname)"
            OK=$?
            if [[ ${OK} == 0 ]]; then
                break
            fi
        done
    fi
    return $OK
}

# Execute command under supervision of zk-flock
function exec_with_zk_lock {
    if ! downtime_host ; then
        log "Cannot downtime host"
        monitoring 1 "Cannot downtime host"
        return 1
    fi
    coproc ZKLOCK {
        zk-flock -w 10 -x 81 -c $ZK_FLOCK_CONFIG $LOCK_NAME "bash -c 'echo Locked; read'"
        if (($? == 81)); then
            echo "LockBusy"
        else
            echo "Failed"
        fi
    }
    local zk_stdin=${ZKLOCK[1]}

    read -u ${ZKLOCK[0]} zk_reply 2>/dev/null
    log "Status from zk-flock: $zk_reply"

    if [[ "$zk_reply" != "Locked" ]]; then
        if [[ "$zk_reply" == "LockBusy" ]]; then
            monitoring 0 "Someone else got zk lock."
            # 81 == LockBusy
            return 81  # zk-flock exit status do not remove or modify this comment
        fi
        monitoring 2 "Failed to get zk lock." >&2
        return 4  # zk-flock unexpected error
    fi
    log "Run $@"
    $@
    ret=$?
    log "Stop zk lock: $(echo "stop zk lock" >&$zk_stdin)"

    remove_downtime || log "Warning: cannot remove downtime"

    if ((ret == 0)); then
        log "Succesfull exec with zk lock"
    fi
    return $ret
}

# Determine if this machine slave or not
function get_slaviness {
    log "Finding out database role"
    if mongo --quiet localhost:$MONGO_DUMP_PORT $MONGODB_AUTHARGS <<<"rs.isMaster().ismaster"|grep -q false;then
        IS_SLAVE=1 # Assume it is hidden slave
        log "This server is slave."
    else
        return # Return if master
    fi
    local hidden=$(mongo --quiet localhost:$MONGO_DUMP_PORT $MONGODB_AUTHARGS <<<"rs.isMaster().hidden"| grep -v bye)
    if test -z "$hidden" || [[ "$hidden" == 'false' ]]; then
        IS_SLAVE=2 # It is normal slave
        log "This server is normal slave."
    fi
}

# Send collected dump to the rsync server
rsync_watcher() {
    local rsync_pid=$1

    trap 'exit 0' SIGTERM
    while grep -q 'mongo-backup' /proc/$$/cmdline &>/dev/null; do
        sleep 30 # time to initialize rsync
        local synced_size=$(RSYNC_PASSWORD=$R_PASSWORD rsync -a --dry-run --stats \
          "rsync://$R_USER@$R_SERVER/$R_REALM/$SETNAME/${DUMPDIR#${SYNCDIR%/*}}/.*" .\
          |awk '/Total transferred file size/{gsub("[^0-9]", "", $5);print $5}')
        if [ ${SYNC_DIR_SIZE:-0} -gt 0 ];then
            local r_progress=$((${synced_size}*100/${SYNC_DIR_SIZE}))
            monitoring 0 "Uploading ${r_progress:-Unknown}% ..." >&2
        fi
        sleep 150 # time to reduce log messages
    done
}

# backup single node (master obviously)
backup_standalone() {
    local target_dir=$1

    log "Dumping standalone with dump_shard function."
    dump_shard $target_dir
    local dump_shard_exit=$?
    if ((MONGOD_VERSION < 320)); then
        log "old mongod, fsyncLock don't give data consistency without stopping"
        # we should stop mongodb in dump_shard because all replica will try stop mongodb
        # and only one got zk lock and really stop mongod
        # but start should be placed here as it should not runned under retry
        log "Starting mongodb $(/sbin/initctl start mongodb 2>&1)"
        if ! /sbin/initctl status mongodb|grep -q running; then
            monitoring 2 "Failed to start mongo."
            EXITCODE=3
        fi
    else
        log "New mongod (> 3.2.0), fsyncLock give data consistency without stopping"
        unlock_cmd='
            var resp = db.getSisterDB("admin").fsyncUnlock();
            while (resp.lockCount > 0) {
                print("WARN: Lock count expect be 0, got " + resp.lockCount + " try unlock.");
                resp = db.getSisterDB("admin").fsyncUnlock();
            }
        '
        log "Run mongo command: $unlock_cmd"
        mongo --ipv6 localhost:$MONGO_DUMP_PORT $MONGODB_AUTHARGS --quiet <<<"$unlock_cmd"
    fi
    return $dump_shard_exit
}


rsync_upload() {
    log "Sending dump to rsync://$R_USER@$R_SERVER (${MAX_RSYNC_RETRIES:=10} retry)"
    SYNC_DIR_SIZE=$(du -bs $SYNCDIR | cut -f1)
    for ((__retry=1;;__retry++)); do
        RSYNC_PASSWORD=$R_PASSWORD rsync -a "$SYNCDIR" \
          "rsync://$R_USER@$R_SERVER/$R_REALM/$SETNAME" >/dev/null &
        RSYNC_PID=$!
        log "Started rsync with pid $RSYNC_PID"

        rsync_watcher &
        WATCHER_PID=$!
        log "Started monitoring process with pid $WATCHER_PID"

        wait $RSYNC_PID
        ret=$?
        log "Stopping monitoring process $WATCHER_PID"
        kill $WATCHER_PID
        if [ $ret == 0 ]; then
            log "rsync successfully sync backup data!"
            break
        else
            if (($__retry >= ${MAX_RSYNC_RETRIES:=10})); then
              monitoring 2 "Failed to rsync dump (${MAX_RSYNC_RETRIES} retries)" >&2
              EXITCODE=4
              break
            fi
            log "rsync exited with non-zero code, trying again($__retry)"
        fi
    done
}

s3cmd_upload() {
    local s3path="s3://$BUCKET/backup/mongo/$SETNAME/"
    log "Sending dump to $s3path"
    dump_size_in_gb=$(du -BG -s "$SYNCDIR" | awk -FG '{print $1 }')
    if (( $dump_size_in_gb > 1000 )); then
        chunk_size=4096
    elif (( $dump_size_in_gb > 500 )); then
        chunk_size=2048
    elif (( $dump_size_in_gb > 250 )); then
        chunk_size=1024
    elif (( $dump_size_in_gb > 125 )); then
        chunk_size=512
    else
        chunk_size=256
    fi

    for ((__retry=1;;__retry++)); do
        coproc {
            s3cmd put --recursive --stats \
                   --multipart-chunk-size-mb=${chunk_size} \
                   "$SYNCDIR" "$s3path" 2>&1
        }
        local s3cmd_pid=$COPROC_PID
        while kill -0 $s3cmd_pid 2>/dev/null; do
            read -t 600 -u ${COPROC[0]} # 10 min timout if s3cmd die without output
            log "$REPLY"
        done
        wait $s3cmd_pid
        if (($? == 0)); then
            log "$TRANSPORT successfully sync backup data!"
            break
        else
            if (($__retry >= ${MAX_RSYNC_RETRIES:=10})); then
              monitoring 2 "Failed to upload $TRANSPORT (${MAX_RSYNC_RETRIES} retries)" >&2
              EXITCODE=4
              break
            fi
            log "$TRANSPORT exited with non-zero code, trying again($__retry)"
        fi
    done
}


#########################
# Here starts real work #
#########################
monitoring -1 "Initialize monitoring file"
if ((! MONGO_BACKUP_ENABLED)); then
    monitoring 0 "Backup disabled"
    exit 0
fi
monitoring 0 "Backup started"

MONGOD_VERSION=$(mongod --version|sed -nr '/^db version/s/[^0-9]//gp' 2>/dev/null)
log "Work on mongod version ${MONGOD_VERSION:=0}"
if test -z "$M_USER" && test -z "$M_PASSWD"; then
    # without auth
    MONGOS_AUTHARGS=""
    MONGODB_AUTHARGS=""
fi

OP_MODE=$STANDALONE  # var standalone contains mongo mode
if ((OP_MODE == MODE_SARDED)); then
    resolve_mongos_host
fi

SYNCDIR="${SYNCDIR:-/tmp/$(date +%Y%m%d)}"
DUMPDIR="$SYNCDIR/$(hostname -f)"

if [[ "x$SHARD_NAME" != "x" ]]; then
    SETNAME="$SHARD_NAME"
else
    SETNAME=$(echo "rs.status().set" | mongo localhost:$MONGO_DUMP_PORT $MONGODB_AUTHARGS --quiet)
    if [[ $? -ne 0 ]]; then
        monitoring 2 "Can not athenticate, exiting"
        exit 1
    fi
    SETNAME=$(echo "$SETNAME" | grep -v '^bye$')
    # For standalone host use hostname
    if [[ "x$SETNAME" == "x" ]]; then
        SETNAME=`hostname -f | sed 's/\./_/g'`
    fi
fi
LOCK_NAME="$SETNAME-dump-lock"
IS_SLAVE=0 # Master by default

get_slaviness;

# We won't dump master if mongo-backup is not configured as standalone
if ((! IS_SLAVE && OP_MODE != MODE_SINGLE)); then
    monitoring 0 "Is master"
    exit 0
fi

# Sleep if it is not hidden slave to prevent backup on normal slave in case
# it is activated there. If hidden slave is down we will get lock and dump
# normal slave.
if (( IS_SLAVE != IS_HIDDEN )); then
    log "Sleeping 50 seconds since it is not hidden slave."
    sleep 50
fi

log "Ensure dump directory $(mkdir -vp "$DUMPDIR")"

if ((OP_MODE == MODE_SINGLE && ! IS_SLAVE)); then
    log "$(backup_standalone $DUMPDIR 2>&1)"
    exit_status=$?
else
    if ((OP_MODE == MODE_SARDED)); then
        lock_config;
        dump_to_bson "$DUMPDIR" config;
        if test -n "$M_USER"; then # if M_USER empty, skip dumping users
            dump_to_bson "$DUMPDIR" admin system.users;
        fi
    fi

    set_oplog_counter
    check_mongodb_state
    # single replica and sharded replica perform backup here
    exec_with_zk_lock "dump_shard $DUMPDIR"
    exit_status=$?

    if ((MONGOD_VERSION < 320)); then
        log "old mongod, fsyncLock don't give data consistency without stopping"
        # we should stop mongodb in dump_shard because all replica will try stop mongodb
        # and only one got zk lock and really stop mongod
        # but start should be placed here as it should not runned under retry
        log "Starting mongodb $(/sbin/initctl start mongodb 2>&1)"
        if ! /sbin/initctl status mongodb|grep -q running; then
            monitoring 2 "Failed to start mongo."
            EXITCODE=3
        fi
    else
        log "New mongod (> 3.2.0), fsyncLock give data consistency without stopping"
        unlock_cmd='
            var resp = db.getSisterDB("admin").fsyncUnlock();
            while (resp.lockCount > 0) {
                print("WARN: Lock count expect be 0, got " + resp.lockCount + " try unlock.");
                resp = db.getSisterDB("admin").fsyncUnlock();
            }
        '
        log "Run mongo command: $unlock_cmd"
        mongo --ipv6 localhost:$MONGO_DUMP_PORT $MONGODB_AUTHARGS --quiet <<<"$unlock_cmd"
    fi

    if ((OP_MODE == MODE_SARDED && exit_status == 0)); then
        unlock_config;
    fi
fi

if ((exit_status != 0)); then
    log "'dump_shard' exit status $exit_status, stopping right now!"
    exit $exit_status
fi

if [[ "${TRANSPORT:=rsync}" == "s3cmd" ]]; then
    s3cmd_upload
else
    rsync_upload
    log "Stop watcher $(kill $WATCHER_PID 2>&1)"
fi


# Clean up
log "Cleaning up. $(rm -rf "$SYNCDIR" 2>&1)"

((EXITCODE == 0)) && monitoring 0 "Backup done successfully"
exit $EXITCODE

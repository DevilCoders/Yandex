#!/bin/bash

set -e

CASS_BACKUP_ENABLED=0
CASS_SCHEMA_BACKUP=0
R_USER='LOGIN_HERE'
R_PASSWORD='PASSWORD_HERE'
R_REALM='backup-cassandra'
R_SERVER='localhost'
R_BWLIMIT=0  # rsync bandwidth limit. "0" means unlimited.
CASS_DATA_DIR='/opt/cassandra/data'
MONITORING_FILE='/var/lib/cassandra-backup/backup.rslt'
LOG_FILE="/var/log/cassandra-backup.log"
DATESTR=`date "+%Y%m%d"`
HOSTNAME=`hostname`
SCHEMA_FILE="/tmp/cass_schema.cql"
MAX_RSYNC_RETRY=3
TRANSPORT=rsync
COMPRESSOR=pigz
SUFFIX=tgz

if test -s /etc/cassandra-backup/cassandra-backup.conf; then
    . /etc/cassandra-backup/cassandra-backup.conf
fi

if [[ "$COMPRESSOR" =~ zst ]]; then
    SUFFIX=tzst
fi

export RSYNC_PASSWORD=$R_PASSWORD

EXITCODE=0

function log_message {
    echo "[$(date +'%Y-%m-%d %H:%M:%S')] $1" >> "$LOG_FILE"
}

function monitoring {
    local level="$1" message="$2"
    if ! [[ $level =~ ^[012]$ ]]; then
        message="Failed, exit status '$level' with '$message'";
        level=2
    fi
    echo "$(date +%s) $level;$message" >> "$MONITORING_FILE"
}

upload_backup(){
    local how=$1 spath="$2" dpath="$3"
    if [[ "${how:-rsync}" == "rsync" ]];then
        upload_rsync "$spath" "$dpath" &>>$LOG_FILE
    else
        upload_s3cmd "$spath" "$dpath" &>>$LOG_FILE
    fi
}

upload_rsync(){
    set -x
    local try=1 err
    local src_path="$1" dst_path="$2"
    if test -z "$dst_path"; then
        src_path="$CASS_DATA_DIR/*/*/$src_path"
    fi

    until rsync --bwlimit=$R_BWLIMIT -ax $src_path \
        "rsync://$R_USER@$R_SERVER/$R_REALM/$dst_path"
    do
        err=$?
        if (( try++ >= MAX_RSYNC_RETRY)); then
            EXITCODE=$((err>1?err:2))
            log_message "Failed to rsync $src_path"
            set +x
            return
        fi
    done
    set +x
    log_message "Upload rsync ok"
}

upload_s3cmd(){
    set -x
    local try=1 err
    local src_path="$1" find_src="$1" find_regex="$1" dst_path="$2"
    if test -z "$dst_path"; then
        # strip leading /opt/cassandra/data/*/*/snapshots/<hostname>/<date>
        local sed_re="s|$CASS_DATA_DIR"'/[^/]\+/[^/]\+'"/$src_path/$DATESTR/||"
        dst_path="${src_path##*/}/$DATESTR/data.${SUFFIX}"
        find_src="$CASS_DATA_DIR"
        # src_path is snapshots/<hostname>
        find_regex=".*/$src_path/$DATESTR/.*"
    else
        # strip leading /tmp
        local sed_re='s|/tmp/||'
        dst_path="$dst_path/${src_path##*/}.${SUFFIX}"
    fi
    local s3path=s3://$BUCKET/backup/cassandra/$dst_path
    until tar -P --null -T <(find $find_src -regex "$find_regex" -print0) \
                --transform="$sed_re" -I "$COMPRESSOR" $strip_path -cf - |\
                s3upload.py $s3path
    do
        err=$?
        if (( try++ >= MAX_RSYNC_RETRY)); then
            EXITCODE=$((err>1?err:2))
            log_message "Failed to upload $s3path"
            set +x
            return
        fi
    done
    set +x
    log_message "Upload s3cmd ok"
}

# START backup check via monrun
if [[ "$1" =~ ^(-m|--monrun)$ ]]; then
    if ((CASS_BACKUP_ENABLED == 0)); then
        echo "0;Backup disabled"
        exit 0
    fi
    MAX_DELTA="${2:-86400}"
    if [ ! -f $MONITORING_FILE ] ; then
        echo "2;Cannot find rslt file: $MONITORING_FILE"
        exit 0
    fi
    LAST_STATUS=`tail -1 $MONITORING_FILE`
    if ((${#LAST_STATUS} < 14)); then
        echo "2;Result file ($MONITORING_FILE) looks corrupted"
        exit 0
    fi
    read TS MSG <<<"$LAST_STATUS"
    DELTA=$(($(date +%s)-$TS))
    if ((DELTA > MAX_DELTA)); then
        echo "2;Backup is too old"
    else
        echo "$MSG"
    fi
    exit 0
fi
# END backup check via monrun

if ((CASS_BACKUP_ENABLED == 0)); then
    monitoring 0 "Backup disabled"
    exit 0
fi

log_message "Starting snapshot(full) backup"

for KEYSPACE in $CASS_BACKUP_KEYSPACES; do
    log_message "Checking keyspace $KEYSPACE"
    if ! nodetool status $KEYSPACE &>/dev/null; then
        EXITCODE=2
        log_message "Failed to check keyspace '$KEYSPACE', skip"
        continue
    fi

    # take snapshot for each table
    for TABLE in $(nodetool cfstats $KEYSPACE|awk '/Table:/{print $2}'); do
        SNAP_NAME="$HOSTNAME/$DATESTR/$KEYSPACE/$TABLE"
        log_message "Creating snapshot for $KEYSPACE.$TABLE"
        if ! nodetool -h localhost snapshot \
            $KEYSPACE -cf $TABLE -t $SNAP_NAME &>> $LOG_FILE
        then
            EXITCODE=2
            log_message "Failed nodetool error on snapshot $KEYSPACE.$TABLE"
        fi
    done
done


# Upload the staff send to backup storage via rsync
log_message "Sending snapshots to backup storage"
upload_backup "$TRANSPORT" "snapshots/$HOSTNAME"

# Clean up
for CASS_KEYSPACE_NAME in $CASS_BACKUP_KEYSPACES ; do
    log_message "Cleaning up: clearsnapshot for $CASS_KEYSPACE_NAME keyspace"
    if !  nodetool -h localhost clearsnapshot \
        $CASS_KEYSPACE_NAME -t "$HOSTNAME" &>> $LOG_FILE
    then
        log_message "Failed to delete keyspace snapshot: $CASS_KEYSPACE_NAME"
        EXITCODE=2
    fi
done

# backup schema
if ((CASS_SCHEMA_BACKUP == 1)); then
    if ! cqlsh -e "DESC SCHEMA" > $SCHEMA_FILE 2>>$LOG_FILE; then
        log_message "Failed to backup SCHEMA"
        EXITCODE=2
    else
        upload_backup "$TRANSPORT" "$SCHEMA_FILE" "$HOSTNAME/$DATESTR"
    fi
    log_message "$(rm -vf $SCHEMA_FILE)"  # clean up
else
    log_message "SCHEME backup up is disabled"
fi

if ((EXITCODE==0)) ; then
    MONITORING_MSG="Snapshot for $DATESTR done successfully"
else
    MONITORING_MSG="Snapshot for $DATESTR done with errors. Check $LOG_FILE"
fi

log_message "$MONITORING_MSG"
monitoring $EXITCODE "$MONITORING_MSG"

exit $EXITCODE

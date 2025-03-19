#!/usr/bin/env bash

timeout="23h"
labels=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --backup-name) backup_name="$2"; shift 2;;
        --sleep)       sleep="$2"; shift 2;;
        --timeout)     timeout="$2"; shift 2;;
        --purge)       purge=1; shift;;
        --force)       force=1; shift;;
        --label)       labels="$labels --label $2 "; shift 2;;
        -h|--help)     help=1; shift; break;;
        *)             help=1; error=1; break;;
    esac
done

if [[ "$help" == 1 ]]; then
    cat <<EOF
Usage: `basename $0` [<option>] ...
  --backup-name                      Backup name
  --sleep, --random-sleep <seconds>  Add random sleep.
  --timeout <seconds>                Timeout for backup creation.
  --purge                            Purge old backups.
  --force                            Enable force mode of backup creation.
  --label                            Custom label for backup.
  -h, --help                         Show this help message and exit.
EOF
    exit ${error:0}
fi

if [[ -n "$sleep" ]]; then
    sleep $(( RANDOM % $sleep ))
fi

backup_opts="$labels"
if [[ -n "$force" ]]; then
    backup_opts+="--force"
fi

# Clean up hung processes from the previous run, if there are any
pkill -9 -f "ch-backup backup"
pkill -9 -f "ch-backup purge"

# Set OOM score higher than it has ClickHouse server.
echo 100 > /proc/self/oom_score_adj

if [[ -z "$backup_name" ]]; then
    backup_name=$(shuf -zer -n20  {a..v} {0..9} | tr -d '\0')
fi

echo "$(date --rfc-3339 seconds) Create new backup: $backup_name" | tee -a /var/log/ch-backup/stdout.log >> /var/log/ch-backup/stderr.log
timeout "$timeout" ch-backup backup --name "$backup_name" $backup_opts >> /var/log/ch-backup/stdout.log 2>> /var/log/ch-backup/stderr.log

if [[ -n "$purge" ]]; then
    echo "$(date --rfc-3339 seconds) Purge old backups per retention policy" | tee -a /var/log/ch-backup/stdout.log >> /var/log/ch-backup/stderr.log
    timeout 30m ch-backup purge >> /var/log/ch-backup/stdout.log 2>> /var/log/ch-backup/stderr.log
fi

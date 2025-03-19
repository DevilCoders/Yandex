#!/bin/bash
pth="/opt/storage"
check_file="$pth/last-backup-status"
lock_file="/tmp/make-backup-of-backup.lock"
MAX_TIME=172800

if [ -f "$check_file" ]; then
  check_file_mtime=`stat -c %Y $check_file`
  currtime=`date +%s`
  diff=$(( currtime - check_file_mtime ))
  if [[ "$diff" -gt "$MAX_TIME" ]]; then
    echo "2;$check_file too old. No backups for $diff seconds. Last backup status: `cat $check_file`"
  else
    echo "`cat $check_file`. Ended $diff seconds ago"
  fi
else
  echo "1;No $check_file file"
fi



#!/bin/bash
pth="/opt/storage"
backup_machines=`curl -s -m 10 "https://c.yandex-team.ru/api-cached/groups2hosts/cult-stable-backup-backup"`
if [ -z "$backup_machines" ]; then
  backup_machines=`cat /var/tmp/backup_machines`
else
  echo $backup_machines > /var/tmp/backup_machines
fi

if [ -n "$backup_machines" ]; then
  if [ -d $pth ]; then
    status=""
    cd $pth
    find . -mtime -1 > $pth/files-to-backup
    for backup in $backup_machines; do
      rsync -qa --no-motd --files-from=./files-to-backup $pth/ $backup::all-backups ; ec=$?
      if [ "$ec" != 0 ]; then
        status="$status;$backup:$ec"
      fi
    done
  fi
fi

if [ -z "$status" ]; then
  echo "0;ok" > $pth/last-backup-status
else
  echo "2;$status" > $pth/last-backup-status
fi

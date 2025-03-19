#!/usr/bin/env bash
set -ex

NBS_HOST=$1

if [ -z "$NBS_HOST" ]
then
  echo "Please enter host in parameters"
  exit -1
fi

DR_ID=$(curl http://$NBS_HOST:8766/blockstore/disk_registry_proxy | \
        grep -Eo 'TabletID=[0-9]*' | grep -Eo '[0-9]*')

if [ -z "$DR_ID" ]
then
  echo "Can't find Disk Registry tablet ID"
  exit -1
fi

function executeAction {
  blockstore-client ExecuteAction --host $NBS_HOST \
                                  --verbose error  \
                                  --action $@
}

echo "Disk Registry ID: $DR_ID"

echo "Setting allocations disallowed"
executeAction AllowDiskAllocation \
              --input-bytes '{"Allow":false}'

echo "Get backup file"
executeAction BackupDiskRegistryState \
              --input-bytes '{"BackupLocalDB":true}' > backup.json

echo "Archive Backup file"
tar -cf backup.tar backup.json

echo "Reset DR tablet $DR_ID"
executeAction ResetTablet \
              --input-bytes '{"TabletId":"'$DR_ID'","Generation":0}'

echo "Restore DR"
executeAction RestoreDiskRegistryState \
              --input backup.json

echo "Kill DR tablet $DR_ID"
executeAction KillTablet \
              --input-bytes '{"TabletId":"'$DR_ID'"}'

echo "Setting allocations allowed"
executeAction AllowDiskAllocation \
              --verbose error --input-bytes '{"Allow":true}'

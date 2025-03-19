#!/usr/bin/env bash

DB_ENDPOINT="grpcs://disk-manager-dn.ydb.cloud-preprod.yandex.net:2136"
DB_NAME="/pre-prod_global/disk-manager"
ZONE_ID="$1"
TABLE_PREFIX="pools"
BASE_DISKS_TABLE="${TABLE_PREFIX}/base_disks"
POOLS_TABLE="${TABLE_PREFIX}/pools"
POOLS_FILE="pools-${ZONE_ID}.json"
IMAGE_IDS_FILE="image_ids-${ZONE_ID}.txt"

function process_pool() {
    echo "processing pool $1" &&
    cat query_template.txt | sed 's#$IMAGE_ID#'"$1"'#g' | sed 's#$ZONE_ID#'"$ZONE_ID"'#g' | sed 's#$BASE_DISKS_TABLE#'"$BASE_DISKS_TABLE"'#g' | sed 's#$POOLS_TABLE#'"$POOLS_TABLE"'#g' > query.txt &&
    for i in `seq 1 10`
    do
        sleep $((i - 1)) &&
        echo "attempt ${i}" &&
        ydb --token-file token --endpoint="$DB_ENDPOINT" -d "$DB_NAME" table query exec -f query.txt &&
        break
    done &&
    echo "finished processing pool $1"
}

ydb --token-file token --endpoint="$DB_ENDPOINT" -d "$DB_NAME" table query exec -t scan -q 'select * from `'"$POOLS_TABLE"'` where status = 0 and zone_id = "'"$ZONE_ID"'"' --format json-unicode > "$POOLS_FILE"

while read line
do
    echo $line | jq -r .image_id
done < "$POOLS_FILE" > "$IMAGE_IDS_FILE"

while read image_id
do
    process_pool "$image_id"
done < "$IMAGE_IDS_FILE"

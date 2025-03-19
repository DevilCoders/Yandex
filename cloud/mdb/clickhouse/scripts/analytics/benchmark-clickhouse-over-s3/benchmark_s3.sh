#!/bin/bash -e

if [[ -n $1 ]]; then
    SCALE=$1
else
    SCALE=100
fi

if [[ ! -n $CLOUD_RUN ]]; then
  docker-compose up -d
fi

QUERY_LIMIT=""
if [[ -n $DATA_LIMIT ]]; then
  QUERY_LIMIT="LIMIT ${DATA_LIMIT}"
fi

if [[ ! -n $STORAGE_POLICY ]]; then
	STORAGE_POLICY='default'
fi

TABLE="hits_${SCALE}m_obfuscated"
QUERIES_FILE="../queries.sql"
TRIES=3

AMD64_BIN_URL="https://builds.clickhouse.tech/master/amd64/clickhouse"
AARCH64_BIN_URL="https://builds.clickhouse.tech/master/aarch64/clickhouse"

# Note: on older Ubuntu versions, 'axel' does not support IPv6. If you are using IPv6-only servers on very old Ubuntu, just don't install 'axel'.

FASTER_DOWNLOAD=wget
if command -v axel >/dev/null; then
    FASTER_DOWNLOAD=axel
else
    echo "It's recommended to install 'axel' for faster downloads."
fi

if command -v pixz >/dev/null; then
    TAR_PARAMS='-Ipixz'
else
    echo "It's recommended to install 'pixz' for faster decompression of the dataset."
fi

mkdir -p clickhouse-benchmark-$SCALE
pushd clickhouse-benchmark-$SCALE

if [[ ! -n $CLOUD_RUN ]]; then
  if [[ ! -f clickhouse ]]; then
      CPU=$(uname -m)
      if [[ ($CPU == x86_64) || ($CPU == amd64) ]]; then
          $FASTER_DOWNLOAD "$AMD64_BIN_URL"
      elif [[ $CPU == aarch64 ]]; then
          $FASTER_DOWNLOAD "$AARCH64_BIN_URL"
      else
          echo "Unsupported CPU type: $CPU"
          exit 1
      fi
  fi
  chmod a+x clickhouse
fi

if [[ ! -n $CLOUD_RUN ]]; then
  ../update_data.sh
fi

uptime

echo "Starting clickhouse-server"
if [[ ! -n $CLOUD_RUN ]]; then
  ./clickhouse server --config=../s3.xml > server.log 2>&1 &
  PID=$!
fi

PORT_OPTION=""
if [[ ! -n $CLOUD_RUN ]]; then
  PORT_OPTION="--port=19000"
fi

function finish {
    kill $PID
    wait
}
trap finish EXIT

if [[ -n $CLOUD_RUN ]]; then
	TABLE="remote('${CLOUD_DATA_HOST}', default.hits_100m_obfuscated, 'admin', '${CLOUD_DATA_ADMIN_PASSWORD}')"
fi

echo "Waiting for clickhouse-server to start"

if [[ -n $CLOUD_RUN ]]; then
  CLICKHOUSE_EXECUTABLE="clickhouse"
else
  CLICKHOUSE_EXECUTABLE="./clickhouse"
fi

for i in {1..30}; do
    sleep 1
    $CLICKHOUSE_EXECUTABLE client $PORT_OPTION --query "SELECT 'The dataset size is: ', count() FROM $TABLE" 2>/dev/null && break || echo '.'
    #if [[ $i == 30 ]]; then exit 1; fi
done

echo
echo "Will perform benchmark. Results:"
echo

# Create
TABLE_S3="hits_100m_hybrid"
echo "DROP TABLE IF EXISTS ${TABLE_S3};" > create_table.sql
echo "CREATE TABLE ${TABLE_S3}" >> create_table.sql
cat ../hits_100m_obfuscated.sql | tail -n+2 | head -n-1 >> create_table.sql
echo "SETTINGS storage_policy='${STORAGE_POLICY}'" >> create_table.sql

$CLICKHOUSE_EXECUTABLE client $PORT_OPTION --max_memory_usage 100G --multiquery < create_table.sql 2>&1
echo "INSERT time:"
$CLICKHOUSE_EXECUTABLE client $PORT_OPTION --max_memory_usage 100G --time --format=Null --query="insert into ${TABLE_S3} select * from ${TABLE} ${QUERY_LIMIT}" 2>&1

TABLE=$TABLE_S3
echo "SELECT times:"
rm -f output.txt
cat "$QUERIES_FILE" | sed "s/{table}/${TABLE}/g" | while read query; do
    sync
    # echo 3 | sudo tee /proc/sys/vm/drop_caches >/dev/null
    for i in $(seq 1 $TRIES); do
        RES=$($CLICKHOUSE_EXECUTABLE client $PORT_OPTION --max_memory_usage 100G --time --format=Null --query="$query" 2>&1 ||:)
        [[ "$?" == "0" ]] && echo -n "${RES}" | tee -a output.txt || echo -n "null" | tee -a output.txt
        [[ "$i" != $TRIES ]] && echo -n ", " | tee -a output.txt
    done
    echo "" | tee -a output.txt
done


echo
echo "Benchmark complete. System info:"
echo

echo '----Version, build id-----------'
$CLICKHOUSE_EXECUTABLE local --query "SELECT format('Version: {}, build id: {}', version(), buildId())"
$CLICKHOUSE_EXECUTABLE local --query "SELECT format('The number of threads is: {}', value) FROM system.settings WHERE name = 'max_threads'" --output-format TSVRaw
$CLICKHOUSE_EXECUTABLE local --query "SELECT format('Current time: {}', toString(now(), 'UTC'))"
echo '----CPU-------------------------'
cat /proc/cpuinfo | grep -i -F 'model name' | uniq
lscpu
echo '----Block Devices---------------'
lsblk
echo '----Disk Free and Total--------'
df -h .
echo '----Memory Free and Total-------'
free -h
echo '----Physical Memory Amount------'
cat /proc/meminfo | grep MemTotal
echo '----RAID Info-------------------'
cat /proc/mdstat
#echo '----PCI-------------------------'
#lspci
#echo '----All Hardware Info-----------'
#lshw
echo '--------------------------------'

echo

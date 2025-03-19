set -e

# Run from chadmin directory
# In order to test you need to set up:
#   1) Clickhouse host
#   2) ZooKeeper host
#   3) Identity for zookeeper cli shell. In a format login:password. Example: clickhouse:X7ui1dXIXXXXXXXXXXXXXXXXXXXXXXXX
# Please note that for some commands ZooKeeper host and identity are not set and they are defined from ClickHouse config
# Default port 2181 is used

CLICKHOUSE_HOST=$1
if [[ -z "$CLICKHOUSE_HOST" ]]; then
   echo "ZooKeeper host is not defined. Please set ZooKeeper host as an argument."
   exit 1
fi
ZOOKEEPER_HOST=$2
ZOOKEEPER_CLI_IDENTITY=$3

ZOOKEEPER_PORT=2181

echo "Get result:"
./chadmin.py $CLICKHOUSE_HOST zookeeper --host $ZOOKEEPER_HOST --port $ZOOKEEPER_PORT get /clickhouse
echo "Stat result:"
echo ./chadmin.py $CLICKHOUSE_HOST zookeeper --host $ZOOKEEPER_HOST --port $ZOOKEEPER_PORT --zkcli_identity "$ZOOKEEPER_CLI_IDENTITY" stat /clickhouse
./chadmin.py $CLICKHOUSE_HOST zookeeper --host $ZOOKEEPER_HOST --port $ZOOKEEPER_PORT --zkcli_identity "$ZOOKEEPER_CLI_IDENTITY" stat /clickhouse

echo "Creating directories."
./chadmin.py $CLICKHOUSE_HOST zookeeper --port $ZOOKEEPER_PORT --zkcli_identity "$ZOOKEEPER_CLI_IDENTITY" create /clickhouse/test
./chadmin.py $CLICKHOUSE_HOST zookeeper --port $ZOOKEEPER_PORT create /clickhouse/test/a /clickhouse/test/b
echo "List result:"
LISTED_DIRECTORIES=$(./chadmin.py $CLICKHOUSE_HOST zookeeper --port $ZOOKEEPER_PORT list /clickhouse/test)
echo $LISTED_DIRECTORIES
if [[ -z "$LISTED_DIRECTORIES" ]]; then
   echo "Error: Directories were not created."
   exit 1
fi

echo "Deleting directories."
./chadmin.py $CLICKHOUSE_HOST zookeeper --port $ZOOKEEPER_PORT delete /clickhouse/test/a
./chadmin.py $CLICKHOUSE_HOST zookeeper --port $ZOOKEEPER_PORT delete /clickhouse/test/b
echo "List result."
LISTED_DIRECTORIES=$(./chadmin.py $CLICKHOUSE_HOST zookeeper --port $ZOOKEEPER_PORT list /clickhouse/test | sed '/^\s*$/d' | wc -l)
echo $LISTED_DIRECTORIES
if [[ $LISTED_DIRECTORIES -ne 0 ]]; then
   echo "Error: Directories were not deleted."
   exit 1
fi

echo "Clear created directories."
./chadmin.py $CLICKHOUSE_HOST zookeeper --port $ZOOKEEPER_PORT delete /clickhouse/test

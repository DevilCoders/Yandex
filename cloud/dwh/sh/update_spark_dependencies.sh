ARC_ROOT=/Users/soin08/arc/arcadia
PYTHON_ROOT=/usr/local/lib/python3.7
DEP_PATH=$ARC_ROOT/cloud/dwh/dependencies.zip

cd "$ARC_ROOT/cloud/dwh"
zip -r -X "$DEP_PATH" spark
cd "$ARC_ROOT/contrib/python/pytz"
zip -ur "$DEP_PATH" pytz
cd "$ARC_ROOT/library/python/vault_client"
zip -ur "$DEP_PATH" vault_client
cd "$ARC_ROOT/contrib/python/paramiko"

cat $DEP_PATH | yt upload //home/cloud_analytics/dwh/spark/dependencies.zip --proxy hahn


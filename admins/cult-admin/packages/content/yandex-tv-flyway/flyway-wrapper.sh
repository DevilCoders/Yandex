#!/bin/sh

# by rikunov@

FLYWAY_BASE_DIR='/opt/flywaydb'
TMPDIR=${TMPDIR:-'/tmp'}

if [ "$#" -lt 2 ]; then
  echo "Usage: $0 dbhost [options] migrate|clean|info|validate|baseline" >&2
  exit 1
fi

DBHOST=$1

echo "Running flyway for: $DBHOST"

FLYWAY_OPTIONS=""

shift
while test ${#} -gt 0
do
  FLYWAY_OPTIONS="$FLYWAY_OPTIONS ${1}"
  shift
done

echo "Flyway options: $FLYWAY_OPTIONS"

echo "Flyway home: $FLYWAY_BASE_DIR"

FLYWAY_PLACEHOLDERS_CONF="$FLYWAY_BASE_DIR/conf/flyway.placeholders.conf"

if [ ! -f $FLYWAY_PLACEHOLDERS_CONFIG ]; then
    echo "Flyway placeholders config not found!"
    exit 2
fi

echo "Flyway placeholders config: $FLYWAY_PLACEHOLDERS_CONF"

FLYWAY_THIS_SESSION_CONF="$TMPDIR/flyway.session.conf"

sed -e "s/\${dbhost}/$DBHOST/" $FLYWAY_PLACEHOLDERS_CONF > $FLYWAY_THIS_SESSION_CONF
echo "Flyway config resolved: " & cat $FLYWAY_THIS_SESSION_CONF

FLYWAY_CLI="$FLYWAY_BASE_DIR/flyway"
FLYWAY_CMD="$FLYWAY_CLI -configFile=`readlink -f $FLYWAY_THIS_SESSION_CONF` $FLYWAY_OPTIONS"

$FLYWAY_CMD

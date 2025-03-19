#!/bin/bash

BACKUP_OPLOG=0

if [[ $BACKUP_OPLOG -eq 0 ]]; then
    exit 0
fi

. /etc/mongo-backup.conf

UPTIME=$(echo "db.serverStatus().uptimeEstimate" | mongo localhost:27018 --quiet | head -1)

if [[ $UPTIME -lt 600 ]]; then
    exit 0
fi

STAMP=$(date +%H:%M)
CUR_POS=$(echo -e "use local;\n var tstamp = db.oplog.rs.find({}, {ts:1}).sort({ts:-1}).limit(1);\n tstamp.forEach( function(x) {print('Timestamp(' + x.ts.t + ', ' + x.ts.i + ')')});" | mongo localhost:27018 --quiet | egrep -v 'local|bye')

OLD_POS=$(cat /var/tmp/mongo-oplog-cursor)

echo -e "
use local;
var x = db.oplog.rs.find({ts : {\$gt: $OLD_POS, \$lte: $CUR_POS}});
x.forEach(
  function (x) 
  {
    if (x.op == 'i') {
      print(x.ns + '.insert(' + tojson(x.o) + ')') 
    } else if (x.op == 'u') { 
      print(x.ns + '.update(' + tojson(x.o2) +', ' + tojson(x.o) + ')') 
    } else if (x.op == 'd') { 
      print(x.ns + '.remove(' + tojson(x.o) + ')') 
    } 
  }
);" | mongo localhost:27018 --quiet | head -n -1 | tail -n +2 > "/var/tmp/mongo-oplog-data$STAMP"

echo "$CUR_POS" > /var/tmp/mongo-oplog-cursor


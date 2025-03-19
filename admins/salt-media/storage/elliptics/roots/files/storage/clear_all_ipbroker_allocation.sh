#!/bin/bash

#set -x

if ! [ -f /usr/bin/jq ]; then
  echo "INFO: no jq found, need install jq."
  apt-get install -y jq >/dev/null
  if ! [ -f /usr/bin/jq ]; then
    echo "ERROR: need jq, install jq: apt-get install jq"
    exit 2;
  fi
fi


IRUN=0
if ubic status cocaine-isolate-daemon | grep -q running; then
  ubic stop cocaine-isolate-daemon
  IRUN=1
fi
CRUN=0
if ubic status cocaine-runtime | grep -q running; then
  CRUN=1
fi

ATOKEN=$(cat /etc/cocaine-isolate-daemon/cocaine-isolate-daemon.conf | jq -r '.["mtn"]["headers"]["Authorization"]')
FQDN=$(hostname -f)
TMPFILE="/tmp/cache.mtn.ipbroker.allocs.tmp"
ISOLATESTATE="/run/isolate.mtn.db"

curl -s -H "Authorization: $ATOKEN" -H "Content-Type: application/json" "http://ip-broker.qloud.yandex.net/api/endpoint/?scheduler=$FQDN" > $TMPFILE
if (( $? != 0 )); then
  echo "ERROR: cant get allocations from ipbroker."
  exit 1
fi

ALLALLOCS=0
RMALLOCS=0

for a in $(cat $TMPFILE | jq -r '.[]["id"]'); do
  let ALLALLOCS++
  curl -s -H "Authorization: $ATOKEN" -XDELETE http://ip-broker.qloud.yandex.net/api/endpoint/$a
  if (( $? != 0 )); then
    echo "WARNING: Cant delete allocation with ID "$a" but continue anyway"
  else
    let RMALLOCS++
  fi
done

echo "INFO: total allocations "$ALLALLOCS
echo "INFO: removed alocations "$RMALLOCS

rm $TMPFILE
rm $ISOLATESTATE

if (($IRUN)); then
  ubic start cocaine-isolate-daemon
fi
if (($CRUN)); then
  ubic restart cocaine-runtime
fi


#/bin/bash

TIMESTEP=86400

if [ ! -f /var/log/nginx/downloader/access.log ]; then
  echo "2; CRIT, file /var/log/nginx/downloader/access.log not found."
  exit 0
fi

altime=$(tail -n100 /var/log/nginx/downloader/access.log | grep 'hash=' | grep -v 'hash=&' | tail -n1 | awk '{print "date -d \""substr($3,11)"\" +%s"}' | sh)
if [ -z "$altime" ]; then
  echo "0; OK, no requests."
  exit 0
fi

if [ ! -f /var/log/downloads_counter.log ]; then
  echo "2; CRIT, file /var/log/downloads_counter.log not found."
  exit 0
fi

cltime=$(tail -n1 /var/log/downloads_counter.log | awk '{print "date -d \""$2" "$3"\" +%s"}' | sh)
if [ -z "$cltime" ]; then
  echo "2; CRIT, no request in /var/log/downloads_counter.log found."
  exit 0
fi

cwtime=$(( $cltime + $TIMESTEP ))
if (( $altime > $cwtime )); then
  echo "2; CRIT, /etc/cron.d/downloader-logbackup not work property." 
  exit 0
fi

echo "0; OK"
exit 0


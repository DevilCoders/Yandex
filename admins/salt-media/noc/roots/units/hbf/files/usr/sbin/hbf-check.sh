#!/bin/sh

servernames="$1"
status_file="$2"
period=5

while true; do
  seen_error=0
  for servername in $servernames; do
    out=$(curl -fs -m $period -H "Host: $servername" http://localhost/ping)
    if [ "$out" != "OK" ]; then
      seen_error=1
    fi
  done
  if [ $seen_error -eq 1 ]; then
    test -e "$status_file" && {
        echo "turning server OFF"
        rm "$status_file"
      }
  else
      test -e "$status_file" || {
        echo "turning server ON"
        touch "$status_file"
      }
    fi
	sleep $period
done

function on_term {
	echo "caught SIGTERM, turning $servername OFF"
	test -e $status_file && rm $status_file
	# wait until balancers remove this RS
	sleep $period
}
trap on_term SIGTERM

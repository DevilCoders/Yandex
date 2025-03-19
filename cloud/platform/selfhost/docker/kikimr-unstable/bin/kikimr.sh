#!/usr/bin/env bash 

trap "echo -e '\nkikimr stopping...'; exit 0" SIGINT SIGTERM

echo "kikimr starting..."
/Berkanavt/kikimr/bin/ydb_recipe start --ydb-working-dir /kikimr --ydb-binary-path /usr/bin/kikimr --ydb-version="kikimr/stable-18-6"

echo "kikimr started"
socat TCP-LISTEN:2135,fork TCP:127.0.0.1:"$(cat /kikimr/kikimr_ports.txt)" &
echo "translate grpc port - 2135 to " "$(cat /kikimr/kikimr_ports.txt)"
netstat -lnptuw

echo "Pres CTRL+C to stop..."
while :
do
    sleep 1
done

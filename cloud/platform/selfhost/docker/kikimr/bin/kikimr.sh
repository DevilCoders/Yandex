#!/usr/bin/env bash 

die() {
	echo -e '\nkikimr stopping...'; 
	exit 0
}

trap die SIGINT SIGTERM

echo "kikimr starting..."
/Berkanavt/kikimr/bin/ydb_recipe start --ydb-working-dir /kikimr --ydb-binary-path /usr/bin/kikimr-fix-grpc.sh --ydb-version="kikimr/stable-18-6"

echo "kikimr started"
echo "Netstat"
netstat -lnptuw

echo "Pres CTRL+C to stop..."

sleep 86000 &
wait 

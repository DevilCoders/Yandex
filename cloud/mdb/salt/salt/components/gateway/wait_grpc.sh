#!/bin/sh

PORT=${PORT:-"4435"}
GRPCURL=${GRPCURL:-"/usr/local/bin/grpcurl"}
DELAY=${DELAY:-"3"}

function success {
	echo "success"
	exit 0
}

for i in $(seq 1 30)
do
	${GRPCURL} -plaintext localhost:${PORT} grpc.health.v1.Health/Check | grep -F '"status": "SERVING"' && success
	sleep ${DELAY}
done
echo "failure"
exit 1

#!/bin/sh -e

if $(dirname $0)/zk_is_ok.sh; then
    if $(dirname $0)/zk_check_outstanding.sh; then
	echo "0;OK";
    else
	echo "2;check failed"
    fi
else
	echo "2;check failed"
fi

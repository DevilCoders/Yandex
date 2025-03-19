#!/bin/sh -e

if ! $(dirname $0)/zk_is_ok.sh; then
	echo "2;imok failed"
	exit 0
fi

if $(dirname $0)/zk_check.sh srvr '^Mode: (leader|follower)$'; then
	echo "0;OK"
else
	echo "2;mode is neither leader nor follower"
fi

# vim: set ts=4 sw=4 et:

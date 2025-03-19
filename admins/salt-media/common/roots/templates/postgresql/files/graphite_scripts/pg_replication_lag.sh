#!/bin/sh

if [ ! -f /var/lib/postgresql/9.*/main/recovery.conf ]
then
	    exit 0
	fi

	export p_db=postgres
	export psql=/usr/bin/psql

	lag=$($psql -U monitor -A -t -c "select extract(epoch from (current_timestamp - ts)) as replication_lag from repl_mon;" $p_db 2>/dev/null)

	echo "pg.db.replica_lag "${lag}

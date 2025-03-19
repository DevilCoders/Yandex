#!/bin/sh

clients=$(timeout 5 /usr/bin/psql -t "host=localhost port=6432 dbname=pgbouncer user=monitor password=monitor" -c "show clients;" | grep 6432 | wc -l)
echo "clients $clients"

/usr/bin/timeout 5 /usr/bin/psql -t "host=localhost port=6432 dbname=pgbouncer user=monitor password=monitor" -c "show stats" | sed 's/\s|\s/\ /g' |\
while read db total_requests _ _ _ avg_req _ _ avg_query
do
    if [ -z "$db" ]; then
        continue;
    fi

    echo "pool.$db.total_requests $total_requests $ts"
    echo "pool.$db.avg_req $avg_req $ts"
    echo "pool.$db.avg_query $avg_query $ts"
done

exit 0

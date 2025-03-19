SHELL=/bin/sh
PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin
MAILTO=''

*/5 * * * * postgres sleep $((RANDOM \% 120)); flock -n -w 10 /tmp/pg_auto_kill_s3_scripts.lock psql -c "SELECT now(), pg_terminate_backend(pid) FROM pg_stat_activity WHERE coalesce(xact_start, query_start) < current_timestamp - '15 minutes'::interval AND application_name LIKE 's3_script_\%'" >> /var/log/s3/pg_auto_kill_s3_scripts.log 2>&1

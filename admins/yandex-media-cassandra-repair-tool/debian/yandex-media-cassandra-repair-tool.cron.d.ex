#
# Regular cron jobs for the yandex-media-cassandra-repair-tool package
#
0 4	* * *	root	[ -x /usr/bin/yandex-media-cassandra-repair-tool_maintenance ] && /usr/bin/yandex-media-cassandra-repair-tool_maintenance

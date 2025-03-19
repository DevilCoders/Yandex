#
# Regular cron jobs for the yandex-media-mongo-dumper package
#
0 4	* * *	root	[ -x /usr/bin/yandex-media-mongo-dumper_maintenance ] && /usr/bin/yandex-media-mongo-dumper_maintenance

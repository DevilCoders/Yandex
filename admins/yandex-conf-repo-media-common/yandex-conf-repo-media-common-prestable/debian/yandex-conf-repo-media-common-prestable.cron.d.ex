#
# Regular cron jobs for the yandex-conf-repo-media-common-prestable package
#
0 4	* * *	root	[ -x /usr/bin/yandex-conf-repo-media-common-prestable_maintenance ] && /usr/bin/yandex-conf-repo-media-common-prestable_maintenance

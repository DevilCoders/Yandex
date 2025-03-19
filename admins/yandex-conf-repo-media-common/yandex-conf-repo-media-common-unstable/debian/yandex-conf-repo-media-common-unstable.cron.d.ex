#
# Regular cron jobs for the yandex-conf-repo-media-common-unstable package
#
0 4	* * *	root	[ -x /usr/bin/yandex-conf-repo-media-common-unstable_maintenance ] && /usr/bin/yandex-conf-repo-media-common-unstable_maintenance

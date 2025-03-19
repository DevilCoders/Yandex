#
# Regular cron jobs for the yandex-conf-repo-media-common-testing package
#
0 4	* * *	root	[ -x /usr/bin/yandex-conf-repo-media-common-testing_maintenance ] && /usr/bin/yandex-conf-repo-media-common-testing_maintenance

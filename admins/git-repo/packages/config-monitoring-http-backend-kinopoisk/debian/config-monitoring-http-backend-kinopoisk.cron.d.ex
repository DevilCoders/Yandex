#
# Regular cron jobs for the config-monitoring-http-backend-kinopoisk package
#
0 4	* * *	root	[ -x /usr/bin/config-monitoring-http-backend-kinopoisk_maintenance ] && /usr/bin/config-monitoring-http-backend-kinopoisk_maintenance

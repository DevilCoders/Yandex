#
# Regular cron jobs for the config-monitoring-http-front package
#
0 4	* * *	root	[ -x /usr/bin/config-monitoring-http-front_maintenance ] && /usr/bin/config-monitoring-http-front_maintenance

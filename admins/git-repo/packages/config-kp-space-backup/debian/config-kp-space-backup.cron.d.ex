#
# Regular cron jobs for the config-kp-space-backup package
#
0 4	* * *	root	[ -x /usr/bin/config-kp-space-backup_maintenance ] && /usr/bin/config-kp-space-backup_maintenance

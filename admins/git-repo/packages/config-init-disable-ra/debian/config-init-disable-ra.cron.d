20 * * * * root flock -x -w 5 /var/lock/disable-ra.lock /etc/init.d/disable-ra restart >/dev/null 2>&1

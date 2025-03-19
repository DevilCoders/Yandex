0 5 * * *   root    sleep $(($RANDOM/10)); /usr/bin/mysql-grants-update-4 -c -v &>/var/log/mysql-grants-4-cron.log || /bin/true

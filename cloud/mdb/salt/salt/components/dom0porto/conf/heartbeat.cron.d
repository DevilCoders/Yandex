MAILTO=''
PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
*/1 * * * * root sleep $((RANDOM \% 20)) && timeout 10 /etc/cron.yandex/heartbeat.py >/dev/null 2>&1

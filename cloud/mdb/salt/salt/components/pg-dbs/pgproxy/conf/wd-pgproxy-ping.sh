SHELL=/bin/bash
PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin
MAILTO=mail-root@yandex-team.ru

*/5 * * * * root sleep $((RANDOM \% 120)); supervisorctl status ping_pgbouncer | grep -q 'FATAL' && ps -ef | grep [u]wsgi | awk '{print $2}' | xargs kill -9 && supervisorctl start ping_pgbouncer


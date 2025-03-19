MAILTO=''
PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
15 * * * * root /usr/local/yandex/remove_old_backups.sh 2>&1 | logger -t 'remove_old_backups'

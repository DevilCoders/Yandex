#!/usr/bin/env bash

#CFDB=cfdb-dbs.kp.yandex.net
CFDB=dbkp-dbs.kp.yandex.net
dt=`date -d'yesterday' +%Y-%m-%d`
#dt=`date +%Y-%m-%d`

mysql -ukinopoisk -pasd123 -h$CFDB kinopoisk -e 'SELECT id, vote, make_date, user_id FROM users_rel_vote WHERE make_date >= CURRENT_DATE() - INTERVAL 1 DAY;' > /home/www/master01h.kp.yandex.net/cf/${dt}-new.csv
cat /home/www/master01h.kp.yandex.net/cf/${dt}-new.csv | grep ${dt} > /home/www/master01h.kp.yandex.net/cf/${dt}-last.csv
gzip /home/www/master01h.kp.yandex.net/cf/${dt}-last.csv
mv /home/www/master01h.kp.yandex.net/cf/${dt}-last.csv.gz /home/www/master01h.kp.yandex.net/cf/${dt}.csv.gz
rm -f /home/www/master01h.kp.yandex.net/cf/${dt}-new.csv

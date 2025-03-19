#!/bin/sh

dump_dir="/opt/backup/mysql/net/yandex/kp/dbkp"
update_host_dev="devdb01h.kp.yandex.net"
update_host_testing="test-ro-dbm.kp.yandex.net"
MYSQL_USER='updatetables'
MYSQL_PASS='rehbwf'
MYSQL_DB='kinopoisk'
date_now=$(date  '+%Y-%m-%d')
day_now=$(date  '+%w')

if [ ${day_now} == "0" ]; then
   echo "Today is saturday.Full Update for dev"
   files=$(find  $dump_dir/$date_now/   -regextype posix-extended  -regex '^(.*)[[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]]-'${MYSQL_DB}'-(.+).sql.gz$' | xargs)
   MYSQL_HOST=$update_host_dev
elif [ ${day_now} == "6" ]; then 
   echo "Today is sunduy. Full Update for testing"
   files=$(find  $dump_dir/$date_now/   -regextype posix-extended  -regex '^(.*)[[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]]-'${MYSQL_DB}'-(.+).sql.gz$' | xargs)
   MYSQL_HOST=$update_host_testing
else
   echo "Update only some tables on testing"
   tablesforupdate=$(curl http://c.yandex-team.ru/api/generator/evgeniysh.kp_backup2testing | grep -Ev '^#'  | xargs  | sed -e 's/ /|/g')
   files=$(find  $dump_dir/$date_now/   -regextype posix-extended  -regex '^(.*)[[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]]-'${MYSQL_DB}'-('${tablesforupdate}').sql.gz' | xargs)   
   MYSQL_HOST=$update_host_testing
fi

for i in $files; do 
   echo "Updating $i"
   zcat $i |  mysql -h ${MYSQL_HOST} -u ${MYSQL_USER} -p${MYSQL_PASS}  ${MYSQL_DB} 
done


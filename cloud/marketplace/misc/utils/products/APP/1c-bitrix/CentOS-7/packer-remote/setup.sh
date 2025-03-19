#!/bin/bash

set -ev

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-webauth.sh
source /opt/yc-marketplace/yc-cms-iptables.sh

rm $0;
rm /etc/cron.d/first-boot

while [ ! -f /var/lib/cloud/instance/boot-finished ]; do
  echo 'Waiting for cloud-init...'
  sleep 5 
done

flush_welcome_body

MYSQL_PASS=`head /dev/urandom | tr -dc A-Za-z0-9 | head -c 13`
MYSQL_OLD_ROOT_PASS=ZkbEsVNb2rXKT
MYSQL_ROOT_PASS=`head /dev/urandom | tr -dc A-Za-z0-9 | head -c 13`
MYSQL_USER=bitrix0
MYSQL_DB=sitemanager

while !(mysqladmin ping)
do
   sleep 3
   echo "waiting for mysql ..."
done

mysql -u root "-p${MYSQL_OLD_ROOT_PASS}" -e "UPDATE mysql.user SET authentication_string=PASSWORD('${MYSQL_ROOT_PASS}') WHERE User='root';FLUSH PRIVILEGES;"
mysql -u root "-p${MYSQL_ROOT_PASS}" -e "GRANT ALL ON ${MYSQL_DB}.* TO '${MYSQL_USER}'@'localhost' IDENTIFIED BY '${MYSQL_PASS}';FLUSH PRIVILEGES;"

#TODO: REPLACE MYSQL PASSWORD IN /home/bitrix/www/bitrix/php_interface and /home/bitrix/www/bitrix/.settings.php
sed -i "s@DBPassword.*@DBPassword = '${MYSQL_PASS}';@" /home/bitrix/www/bitrix/php_interface/dbconn.php
sed -i "s@'password'.*@'password' => '${MYSQL_PASS}',@" /home/bitrix/www/bitrix/.settings.php
sed -i "s@password.*@password='${MYSQL_ROOT_PASS}'@" /root/.my.cnf


cat > /root/default_passwords.txt <<EOF
MYSQL_PASS=${MYSQL_PASS}
MYSQL_ROOT_PASS=${MYSQL_ROOT_PASS}
MYSQL_USER=${MYSQL_USER}
MYSQL_DB=${MYSQL_DB}
EOF


generate_iptables

run_print_credentials

echo "yc-setup: Done"

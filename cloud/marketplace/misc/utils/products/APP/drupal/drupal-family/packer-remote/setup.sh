#!/bin/bash

set -ev

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-webauth.sh
source /opt/yc-marketplace/yc-cms-iptables.sh

while [ ! -f /var/lib/cloud/instance/boot-finished ]; do echo 'Waiting for cloud-init...'; sleep 5; done
echo "Setup start"

rm $0;
rm /etc/cron.d/first-boot

flush_welcome_body

MYSQL_PASS=`head /dev/urandom | tr -dc A-Za-z0-9 | head -c 13`
MYSQL_OLD_ROOT_PASS=ZkbEsVNb2rXKT
MYSQL_ROOT_PASS=`head /dev/urandom | tr -dc A-Za-z0-9 | head -c 13`
MYSQL_USER=drupal
MYSQL_DB=drupal

echo "2" > /tmp/yc-counter

/etc/init.d/mysql start

mysql -u root "-p${MYSQL_OLD_ROOT_PASS}" -e "UPDATE mysql.user SET authentication_string=PASSWORD('${MYSQL_ROOT_PASS}') WHERE User='root';FLUSH PRIVILEGES;"
mysql -u root "-p${MYSQL_ROOT_PASS}" -e "CREATE DATABASE ${MYSQL_DB} DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci;"
mysql -u root "-p${MYSQL_ROOT_PASS}" -e "GRANT ALL ON ${MYSQL_DB}.* TO '${MYSQL_USER}'@'localhost' IDENTIFIED BY '${MYSQL_PASS}';FLUSH PRIVILEGES;"

cat > /root/default_passwords.txt <<EOF
MYSQL_PASS=${MYSQL_PASS}
MYSQL_ROOT_PASS=${MYSQL_ROOT_PASS}
MYSQL_USER=${MYSQL_USER}
MYSQL_DB=${MYSQL_DB}
EOF

run_print_credentials

chmod 600 /root/default_passwords.txt

#echo "yc-setup: Configure iptables"
generate_iptables
netfilter-persistent save

YC_WELCOME_BODY=`cat <<EOF
You can view generated credentials in /root/default_passwords.txt
Site directory is in /usr/share/drupal/

${YC_WELCOME_BODY}
EOF`

print_msg;

echo "${APACHE2_ADMIN_PASS}" | htpasswd -i /usr/local/apache/passwd/passwords admin
chown -R www-data /usr/share/drupal/

echo "yc-setup: Done"


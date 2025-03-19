#!/bin/bash

set -ev

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-webauth.sh
source /opt/yc-marketplace/yc-cms-iptables.sh

echo "Setup start"

rm $0;
rm /etc/cron.d/first-boot

flush_welcome_body

MYSQL_PASS=`head /dev/urandom | tr -dc A-Za-z0-9 | head -c 13`
MYSQL_OLD_ROOT_PASS=ZkbEsVNb2rXKT
MYSQL_ROOT_PASS=`head /dev/urandom | tr -dc A-Za-z0-9 | head -c 13`
MYSQL_USER=opencart
MYSQL_DB=opencart

echo "yc-setup: Configure iptables"

generate_iptables
netfilter-persistent save

/etc/init.d/mysql start

mysql -u root "-p${MYSQL_OLD_ROOT_PASS}" -e "UPDATE mysql.user SET authentication_string=PASSWORD('${MYSQL_ROOT_PASS}') WHERE User='root';FLUSH PRIVILEGES;"
mysql -u root "-p${MYSQL_ROOT_PASS}" -e "CREATE DATABASE ${MYSQL_DB} DEFAULT CHARACTER SET utf8 COLLATE utf8_unicode_ci;"
mysql -u root "-p${MYSQL_ROOT_PASS}" -e "GRANT ALL ON ${MYSQL_DB}.* TO '${MYSQL_USER}'@'localhost' IDENTIFIED BY '${MYSQL_PASS}';FLUSH PRIVILEGES;"

cat > /root/default_passwords.txt <<EOF
MYSQL_USER=${MYSQL_USER}
MYSQL_PASS=${MYSQL_PASS}
MYSQL_ROOT_PASS=${MYSQL_ROOT_PASS}
MYSQL_DB=${MYSQL_DB}

EOF

run_print_credentials

chmod 600 /root/default_passwords.txt

YC_WELCOME_BODY=`cat <<EOF
You can view generated credentials in /root/default_passwords.txt

${YC_WELCOME_BODY}
EOF`

print_msg;

systemctl start apache2

echo "yc-setup: Done"


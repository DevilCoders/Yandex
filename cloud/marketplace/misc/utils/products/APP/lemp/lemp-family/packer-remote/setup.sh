#!/bin/bash

set -ev

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-cms-iptables.sh

echo "Setup start"

rm $0;
rm /etc/cron.d/first-boot

flush_welcome_body;
generate_iptables;
netfilter-persistent save

MYSQL_OLD_ROOT_PASS=ZkbEsVNb2rXKT
MYSQL_ROOT_PASS=`head /dev/urandom | tr -dc A-Za-z0-9 | head -c 13`

/etc/init.d/mysql start

mysql -u root "-p${MYSQL_OLD_ROOT_PASS}" -e "UPDATE mysql.user SET authentication_string=PASSWORD('${MYSQL_ROOT_PASS}') WHERE User='root';FLUSH PRIVILEGES;"

cat > /root/default_passwords.txt <<EOF
MYSQL_ROOT_PASS=${MYSQL_ROOT_PASS}

EOF

run_print_credentials

YC_WELCOME_BODY=`cat <<EOF
You can view generated credentials in /root/default_passwords.txt

${YC_WELCOME_BODY}
EOF`

rm -rf /opt/yc-marketplace/assets
print_msg;

echo "yc-setup: Done"

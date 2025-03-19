#!/bin/bash

set -ev

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-webauth.sh
source /opt/yc-marketplace/yc-cms-iptables.sh

echo "Setup start"

echo "Adding users to rvm group"
for user in /home/*
do
    usermod -aG rvm $(basename "$user")
done

echo "Set locale to en_US.UTF-8"
update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8

rm $0;
rm /etc/cron.d/first-boot

echo "Regenerate certs with new hostname"
#make-ssl-cert generate-default-snakeoil --force-overwrite

flush_welcome_body

MYSQL_PASS=`head /dev/urandom | tr -dc A-Za-z0-9 | head -c 13`
MYSQL_OLD_ROOT_PASS=ZkbEsVNb2rXKT
MYSQL_ROOT_PASS=`head /dev/urandom | tr -dc A-Za-z0-9 | head -c 13`
MYSQL_USER=testapp
MYSQL_DB=testapp

echo "yc-setup: Configure iptables"

generate_iptables
netfilter-persistent save

/etc/init.d/mysql start

mysql -u root "-p${MYSQL_OLD_ROOT_PASS}" -e "UPDATE mysql.user SET authentication_string=PASSWORD('${MYSQL_ROOT_PASS}') WHERE User='root';FLUSH PRIVILEGES;"
mysql -u root "-p${MYSQL_ROOT_PASS}" -e "CREATE DATABASE ${MYSQL_DB} DEFAULT CHARACTER SET utf8 COLLATE utf8_unicode_ci;"
mysql -u root "-p${MYSQL_ROOT_PASS}" -e "GRANT ALL ON ${MYSQL_DB}.* TO '${MYSQL_USER}'@'localhost' IDENTIFIED BY '${MYSQL_PASS}';FLUSH PRIVILEGES;"

cat > /root/default_passwords.txt <<EOF
MYSQL_PASS=${MYSQL_PASS}
MYSQL_ROOT_PASS=${MYSQL_ROOT_PASS}
MYSQL_USER=${MYSQL_USER}
MYSQL_DB=${MYSQL_DB}
EOF

chgrp rvm /usr/share/ruby-projects/
chmod 0775 /usr/share/ruby-projects/

source /etc/profile.d/rvm.sh

echo "Creating test app"
cd /usr/share/ruby-projects
rails new testapp -d mysql
sed -i "0,/password:/s/password:/password:\ ${MYSQL_ROOT_PASS}/" /usr/share/ruby-projects/testapp/config/database.yml
cd testapp
rake db:create

run_print_credentials

chmod 600 /root/default_passwords.txt

YC_WELCOME_BODY=`cat <<EOF
You can view generated credentials in /root/default_passwords.txt

${YC_WELCOME_BODY}
EOF`

print_msg;

chown -R www-data /usr/share/ruby-projects/
systemctl enable apache2
systemctl restart apache2

rm -rf /opt/yc-marketplace/assets
echo "yc-setup: Done"

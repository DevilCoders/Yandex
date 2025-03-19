#!/bin/bash

set -ve

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-clean.sh

echo "Install packages ..."

apt-get -y update

sleep 10
apt-get -q -y install apache2 libapache2-mod-php mysql-client php-mysql php7.2-common cron debconf-utils iptables php-zip php-curl php7.2-gd php7.2-intl libapache2-mod-security2


MYSQL_ROOT_PASS=ZkbEsVNb2rXKT

echo iptables-persistent iptables-persistent/autosave_v4 boolean false | debconf-set-selections
echo iptables-persistent iptables-persistent/autosave_v6 boolean false | debconf-set-selections
debconf-set-selections <<< "mysql-server-5.7 mysql-server/root_password password ${MYSQL_ROOT_PASS}"
debconf-set-selections <<< "mysql-server-5.7 mysql-server/root_password_again password ${MYSQL_ROOT_PASS}"

apt-get -q -y install iptables-persistent mysql-server-5.7

echo "Setup apache2 ..."

cat > /etc/apache2/sites-enabled/000-default.conf <<EOF
<VirtualHost *:80 [::]:80>
        ServerAdmin webmaster@localhost
        DocumentRoot /var/www/html

        ErrorLog \${APACHE_LOG_DIR}/error.log
        CustomLog \${APACHE_LOG_DIR}/access.log combined
</VirtualHost>
EOF

a2enmod rewrite

generate_print_credentials;
on_install;
print_msg;

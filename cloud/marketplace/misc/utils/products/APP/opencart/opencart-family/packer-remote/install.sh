#!/bin/bash

set -e

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-clean.sh

echo "Install packages ..."
systemd-run --property="After=apt-daily.service apt-daily-upgrade.service" --wait /bin/true

apt-get -y update

apt-get -q -y install apache2 libapache2-mod-php \
    mysql-client php-mysql php-mail \
    php7.2-common cron debconf-utils sendmail \
    unzip iptables php-zip php7.2-curl php7.2-gd php7.2-intl \
    php7.2-zip php7.2-xml php7.2-dom

systemctl stop apache2

MYSQL_ROOT_PASS=ZkbEsVNb2rXKT

echo iptables-persistent iptables-persistent/autosave_v4 boolean false | debconf-set-selections
echo iptables-persistent iptables-persistent/autosave_v6 boolean false | debconf-set-selections
debconf-set-selections <<< "mysql-server-5.7 mysql-server/root_password password ${MYSQL_ROOT_PASS}"
debconf-set-selections <<< "mysql-server-5.7 mysql-server/root_password_again password ${MYSQL_ROOT_PASS}"

apt-get -q -y install iptables-persistent mysql-server-5.7

rm -rf /var/www/html
mkdir /tmp/opencart-install
wget --retry-connrefused --waitretry=1 --read-timeout=20 --timeout=15 -t 3 https://github.com/opencart/opencart/releases/download/3.0.3.1/opencart-3.0.3.1.zip -O /tmp/opencart-install/opencart.zip
unzip /tmp/opencart-install/opencart.zip -d /tmp/opencart-install/

mkdir /var/www/opencart
cp -R /tmp/opencart-install/upload/* /var/www/opencart
mv /var/www/opencart/config-dist.php /var/www/opencart/config.php
mv /var/www/opencart/admin/config-dist.php /var/www/opencart/admin/config.php


rm -rf /tmp/opencart-install

echo "Setup apache2 ..."

cat > /etc/apache2/sites-enabled/000-default.conf <<EOF
<VirtualHost *:80 [::]:80>
        ServerAdmin webmaster@localhost
        DocumentRoot /var/www/opencart

        <Directory /var/www/opencart>
                Options Indexes FollowSymLinks
                AllowOverride All
                Require all granted
        </Directory>

        ErrorLog \${APACHE_LOG_DIR}/error.log
        CustomLog \${APACHE_LOG_DIR}/access.log combined

</VirtualHost>
EOF

a2enmod rewrite

chown -R www-data:www-data /var/www/opencart/

generate_print_credentials;
on_install;
print_msg;


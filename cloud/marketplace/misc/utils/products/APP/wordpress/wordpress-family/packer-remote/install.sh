#!/bin/bash

set -e

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-clean.sh

echo "Install packages ..."
systemd-run --property="After=apt-daily.service apt-daily-upgrade.service" --wait /bin/true
apt-get -y update

mkdir -p /usr/share/
wget --retry-connrefused --waitretry=1 --read-timeout=20 --timeout=15 -t 5 https://wordpress.org/latest.tar.gz -O /usr/share/wordpress.tar.gz
(cd /usr/share/ && tar -xf wordpress.tar.gz)

rm /usr/share/wordpress.tar.gz

apt-get -q -y install apache2 libapache2-mod-php \
    mysql-client php-mysql php-mail \
    php7.2-common cron debconf-utils sendmail \
    unzip iptables

wget --retry-connrefused --waitretry=1 --read-timeout=20 --timeout=15 -t 5 https://downloads.wordpress.org/plugin/wp-mail-smtp.zip -O wp-mail-smtp.zip
unzip wp-mail-smtp.zip
apt-get -y purge unzip

mkdir -p /usr/share/wordpress/wp-content/plugins/
mv wp-mail-smtp /usr/share/wordpress/wp-content/plugins/
rm wp-mail-smtp.zip

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
        DocumentRoot /usr/share/wordpress/

        <Directory /usr/share/wordpress>
                Options Indexes FollowSymLinks
                AllowOverride All
                Require all granted
        </Directory>

        ErrorLog \${APACHE_LOG_DIR}/error.log
        CustomLog \${APACHE_LOG_DIR}/access.log combined

</VirtualHost>
EOF

a2enmod rewrite

mkdir -p /usr/local/apache/passwd/
touch /usr/local/apache/passwd/passwords

chown -R www-data /usr/share/wordpress/

generate_print_credentials;
on_install;
print_msg;


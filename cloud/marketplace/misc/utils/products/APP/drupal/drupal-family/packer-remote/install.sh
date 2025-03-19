#!/bin/bash

set -ev

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-clean.sh

echo "Install packages ..."

systemd-run --property="After=apt-daily.service apt-daily-upgrade.service" --wait /bin/true
apt-get -y update

mkdir -p /usr/share/
wget --retry-connrefused --waitretry=1 --read-timeout=20 --timeout=15 -t 3 https://www.drupal.org/download-latest/tar.gz -O /usr/share/drupal.tar.gz
tar -xf /usr/share/drupal.tar.gz -C /usr/share
mv /usr/share/drupal-* /usr/share/drupal

rm /usr/share/drupal.tar.gz

ls -la /usr/share/ | grep drupal

apt-get -q -y install apache2 \
    libapache2-mod-php7.2 \
    mysql-client \
    php-mail \
    cron \
    debconf-utils \
    sendmail \
    unzip \
    iptables \
    php7.2 \
    php7.2-common \
    php7.2-gd \
    php7.2-curl \
    php7.2-mbstring \
    php7.2-cli \
    php7.2-fpm \
    php7.2-json \
    php7.2-mysql \
    php7.2-opcache \
    php7.2-readline \
    php7.2-xml \
    composer

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
        DocumentRoot /usr/share/drupal/

        ErrorLog \${APACHE_LOG_DIR}/error.log
        CustomLog \${APACHE_LOG_DIR}/access.log combined

#        <Location "/core/install.php">
#            AuthType Basic
#            AuthName "WP Admin page"
#            AuthUserFile /usr/local/apache/passwd/passwords
#            Require valid-user
#        </Location>
</VirtualHost>
EOF

cat >> /etc/apache2/apache2.conf <<EOF
<Directory "/usr/share/drupal/">
  AllowOverride All
  Allow from All
</Directory>
EOF

a2enmod rewrite

mkdir -p /usr/local/apache/passwd/
touch /usr/local/apache/passwd/passwords
chown -R www-data /usr/share/drupal/

generate_print_credentials;
on_install;
print_msg;


#!/bin/bash

set -e

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-clean.sh

echo "Install packages ..."
systemd-run --property="After=apt-daily.service apt-daily-upgrade.service" --wait /bin/true
apt-get -y update

mkdir -p /var/www/html/

curl https://downloads.joomla.org/cms/joomla3/3-9-15/Joomla_3-9-15-Stable-Full_Package.tar.gz?format=gz -o joomla.tar.gz -L
mv joomla.tar.gz /var/www/html/
(cd /var/www/html/ && tar -zxvf joomla.tar.gz)
rm /var/www/html/joomla.tar.gz

apt-get -q -y install php7.2-mysql php7.2-curl php7.2-json php7.2-cgi php7.2 libapache2-mod-php7.2 apache2 mysql-client php-mysql php-mail php7.2-common cron debconf-utils sendmail unzip iptables composer


MYSQL_ROOT_PASS=XnwesNIF7dtD4

echo iptables-persistent iptables-persistent/autosave_v4 boolean false | debconf-set-selections
echo iptables-persistent iptables-persistent/autosave_v6 boolean false | debconf-set-selections
debconf-set-selections <<< "mysql-server-5.7 mysql-server/root_password password ${MYSQL_ROOT_PASS}"
debconf-set-selections <<< "mysql-server-5.7 mysql-server/root_password_again password ${MYSQL_ROOT_PASS}"

apt-get -q -y install iptables-persistent mysql-server-5.7

composer global require joomlatools/console

echo "Setup apache2 ..."

cat > /etc/apache2/sites-enabled/000-default.conf <<EOF
<VirtualHost *:80 [::]:80>
        ServerAdmin webmaster@localhost
        DocumentRoot /var/www/html/

        <Directory /var/www/html/>
                DirectoryIndex index.php index.html
                DirectorySlash off
                RewriteEngine on
                RewriteBase /
                AllowOverride all
        </Directory>

#        <Location "/installation/">
#            AuthType Basic
#            AuthName "Joomla Admin page"
#            AuthUserFile /usr/local/apache/passwd/passwords
#            Require valid-user
#        </Location>

        ErrorLog \${APACHE_LOG_DIR}/error.log
        CustomLog \${APACHE_LOG_DIR}/access.log combined
</VirtualHost>
EOF

a2enmod php7.2
a2dismod mpm_event
a2enmod mpm_prefork
a2enmod rewrite

mv /var/www/html/htaccess.txt /var/www/html/.htaccess
mkdir -p /usr/local/apache/passwd/
touch /usr/local/apache/passwd/passwords
chown -R www-data /var/www/html/

generate_print_credentials;
on_install;
print_msg;

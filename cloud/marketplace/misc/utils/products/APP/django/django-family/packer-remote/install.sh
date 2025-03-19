#!/bin/bash

set -e

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-clean.sh

echo "Install packages ..."

systemd-run --property="After=apt-daily.service apt-daily-upgrade.service" --wait /bin/true
apt-get -y update

mkdir -p /usr/share/django-projects

echo "Updating locales ..."
export LC_ALL=en_US.UTF-8

apt-get -q -y install apache2 libapache2-mod-wsgi-py3 \
    mysql-client libmysqlclient-dev \
    python3-pip python3-dev\
    cron debconf-utils \
    libssl-dev \
    iptables

echo "Install Django with pip3 ..."
pip3 install django

cd /usr/share/django-projects/
django-admin startproject welcome

MYSQL_ROOT_PASS=ZkbEsVNb2rXKT

echo iptables-persistent iptables-persistent/autosave_v4 boolean false | debconf-set-selections
echo iptables-persistent iptables-persistent/autosave_v6 boolean false | debconf-set-selections
debconf-set-selections <<< "mysql-server-5.7 mysql-server/root_password password ${MYSQL_ROOT_PASS}"
debconf-set-selections <<< "mysql-server-5.7 mysql-server/root_password_again password ${MYSQL_ROOT_PASS}"

apt-get -q -y install iptables-persistent mysql-server-5.7

pip3 install mysqlclient

mkdir /usr/share/django-projects/welcome/static

echo "Setup apache2 ..."

cat > /etc/apache2/sites-enabled/000-default.conf <<EOF
<VirtualHost *:80 [::]:80>
        ServerAdmin webmaster@localhost
        DocumentRoot /usr/share/django-projects/

        ErrorLog \${APACHE_LOG_DIR}/error.log
        CustomLog \${APACHE_LOG_DIR}/access.log combined

        WSGIDaemonProcess welcome user=www-data group=www-data python-path=/usr/share/django-projects/welcome
        WSGIProcessGroup welcome
        WSGIScriptAlias / /usr/share/django-projects/welcome/welcome/wsgi.py

        <Directory /usr/share/django-projects/welcome/welcome>
            <Files wsgi.py>
                Require all granted
            </Files>
        </Directory>
        
       Alias /static/ /usr/share/django-projects/welcome/static/

       <Directory /usr/share/django-projects/welcome/static>
            Require all granted
       </Directory>
</VirtualHost>

EOF

sed -i "s/ALLOWED_HOSTS\ =\ \[\]/ALLOWED_HOSTS\ =\ \['*'\]/" /usr/share/django-projects/welcome/welcome/settings.py
sed -i "/STATIC_URL/a STATIC_ROOT\ =\ '/usr/share/django-projects/welcome/static/'" /usr/share/django-projects/welcome/welcome/settings.py

mkdir -p /usr/local/apache/passwd/
touch /usr/local/apache/passwd/passwords

chown -R www-data /usr/share/django-projects/

generate_print_credentials;
on_install;
print_msg;

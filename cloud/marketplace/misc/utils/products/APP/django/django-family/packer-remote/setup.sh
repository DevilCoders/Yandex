#!/bin/bash

set -ev

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-webauth.sh
source /opt/yc-marketplace/yc-cms-iptables.sh

while [ ! -f /var/lib/cloud/instance/boot-finished ]; do echo 'Waiting for cloud-init...'; sleep 5; done

echo "Setup start"

echo "Set locale to en_US.UTF-8"
update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8

rm $0;
rm /etc/cron.d/first-boot


flush_welcome_body

MYSQL_PASS=`head /dev/urandom | tr -dc A-Za-z0-9 | head -c 13`
MYSQL_OLD_ROOT_PASS=ZkbEsVNb2rXKT
MYSQL_ROOT_PASS=`head /dev/urandom | tr -dc A-Za-z0-9 | head -c 13`
MYSQL_USER=django
MYSQL_DB=django
APACHE2_DJANGO_PASS=`curl http://169.254.169.254/latest/meta-data/instance-id`

echo "yc-setup: Configure iptables"

generate_iptables
netfilter-persistent save

/etc/init.d/mysql start

mysql -u root "-p${MYSQL_OLD_ROOT_PASS}" -e "UPDATE mysql.user SET authentication_string=PASSWORD('${MYSQL_ROOT_PASS}') WHERE User='root';FLUSH PRIVILEGES;"
mysql -u root "-p${MYSQL_ROOT_PASS}" -e "CREATE DATABASE ${MYSQL_DB} DEFAULT CHARACTER SET utf8 COLLATE utf8_unicode_ci;"
mysql -u root "-p${MYSQL_ROOT_PASS}" -e "GRANT ALL ON ${MYSQL_DB}.* TO '${MYSQL_USER}'@'localhost' IDENTIFIED BY '${MYSQL_PASS}';FLUSH PRIVILEGES;"

echo "${APACHE2_DJANGO_PASS}" | htpasswd -i /usr/local/apache/passwd/passwords admin

sed -i '/DATABASES/,+6d' /usr/share/django-projects/welcome/welcome/settings.py

cat >> /usr/share/django-projects/welcome/welcome/settings.py<<EOF
DATABASES = {
    'default': {
        'ENGINE': 'django.db.backends.mysql',
        'NAME': '${MYSQL_DB}',
        'USER': '${MYSQL_USER}',
        'HOST': 'localhost',
        'PASSWORD': '${MYSQL_PASS}',
    }
}
EOF

cat > /root/default_passwords.txt <<EOF
MYSQL_PASS=${MYSQL_PASS}
MYSQL_ROOT_PASS=${MYSQL_ROOT_PASS}
MYSQL_USER=${MYSQL_USER}
MYSQL_DB=${MYSQL_DB}

Django Web Auth:
  login: admin
  password: ${APACHE2_DJANGO_PASS}

EOF

systemctl restart apache2
run_print_credentials

chmod 600 /root/default_passwords.txt

YC_WELCOME_BODY=`cat <<EOF
You can view generated credentials in /root/default_passwords.txt
Site directory is in /usr/share/django-projects/

${YC_WELCOME_BODY}
EOF`

print_msg;

cd /usr/share/django-projects/welcome

echo "Applying database migrations"
python3 manage.py migrate

echo "Collecting static files"
python3 manage.py collectstatic --noinput

echo "Set up admin user"
python3 manage.py shell -c "from django.contrib.auth.models import User; User.objects.create_superuser('admin', 'admin@example.com', '${APACHE2_DJANGO_PASS}')"

chown -R www-data /usr/share/django-projects/

echo "yc-setup: Done"

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
MYSQL_USER=wordpress
MYSQL_DB=wordpress
APACHE2_WP_PASS=`curl http://169.254.169.254/latest/meta-data/instance-id`

#echo "yc-setup: Configure iptables"

generate_iptables
netfilter-persistent save

cat > /usr/share/wordpress/wp-config.php <<EOF
<?php

define('DB_NAME', '${MYSQL_DB}');
define('DB_USER', '${MYSQL_USER}');
define('DB_PASSWORD', '${MYSQL_PASS}');
define('DB_HOST', 'localhost');
define('DB_CHARSET', 'utf8');
define('DB_COLLATE', '');

\$table_prefix  = 'wp_';

define('WP_DEBUG', false);

EOF

echo "2" > /tmp/yc-counter

wget --retry-connrefused --waitretry=1 --read-timeout=20 --timeout=15 -t 5 https://api.wordpress.org/secret-key/1.1/salt/ -O /tmp/wp-config

cat /tmp/wp-config >> /usr/share/wordpress/wp-config.php

cat >> /usr/share/wordpress/wp-config.php <<EOF

define('WP_CORE_UPDATE', 'minor');

if ( !defined('ABSPATH') )
	define('ABSPATH', dirname(__FILE__) . '/');

define('FS_METHOD', 'direct');

require_once(ABSPATH . 'wp-settings.php');
EOF

/etc/init.d/mysql start

mysql -u root "-p${MYSQL_OLD_ROOT_PASS}" -e "UPDATE mysql.user SET authentication_string=PASSWORD('${MYSQL_ROOT_PASS}') WHERE User='root';FLUSH PRIVILEGES;"
mysql -u root "-p${MYSQL_ROOT_PASS}" -e "CREATE DATABASE ${MYSQL_DB} DEFAULT CHARACTER SET utf8 COLLATE utf8_unicode_ci;"
mysql -u root "-p${MYSQL_ROOT_PASS}" -e "GRANT ALL ON ${MYSQL_DB}.* TO '${MYSQL_USER}'@'localhost' IDENTIFIED BY '${MYSQL_PASS}';FLUSH PRIVILEGES;"

echo "${APACHE2_WP_PASS}" | htpasswd -i /usr/local/apache/passwd/passwords admin

cat > /root/default_passwords.txt <<EOF
MYSQL_PASS=${MYSQL_PASS}
MYSQL_ROOT_PASS=${MYSQL_ROOT_PASS}
MYSQL_USER=${MYSQL_USER}
MYSQL_DB=${MYSQL_DB}

Apache Web Auth:
  login: admin
  password: ${APACHE2_WP_PASS}

EOF

run_print_credentials

chmod 600 /root/default_passwords.txt

YC_WELCOME_BODY=`cat <<EOF
You can view generated credentials in /root/default_passwords.txt

${YC_WELCOME_BODY}
EOF`

print_msg;

chown -R www-data /usr/share/wordpress/

echo "yc-setup: Done"

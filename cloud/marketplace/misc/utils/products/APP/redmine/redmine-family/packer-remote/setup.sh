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

source /etc/profile.d/rvm.sh
echo "Set locale to en_US.UTF-8"
update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8

rm $0;
rm /etc/cron.d/first-boot

flush_welcome_body

MYSQL_PASS=`head /dev/urandom | tr -dc A-Za-z0-9 | head -c 13`
MYSQL_OLD_ROOT_PASS=ZkbEsVNb2rXKT
MYSQL_ROOT_PASS=`head /dev/urandom | tr -dc A-Za-z0-9 | head -c 13`
MYSQL_USER=redmine
MYSQL_DB=redmine

echo "yc-setup: Configure iptables"

generate_iptables
netfilter-persistent save

systemctl start mysql

mysql -u root "-p${MYSQL_OLD_ROOT_PASS}" -e "UPDATE mysql.user SET authentication_string=PASSWORD('${MYSQL_ROOT_PASS}') WHERE User='root';FLUSH PRIVILEGES;"
mysql -u root "-p${MYSQL_ROOT_PASS}" -e "CREATE DATABASE ${MYSQL_DB} DEFAULT CHARACTER SET utf8 COLLATE utf8_unicode_ci;"
mysql -u root "-p${MYSQL_ROOT_PASS}" -e "GRANT ALL ON ${MYSQL_DB}.* TO '${MYSQL_USER}'@'localhost' IDENTIFIED BY '${MYSQL_PASS}';FLUSH PRIVILEGES;"

cp /usr/share/redmine/config/configuration.yml.example /usr/share/redmine/config/configuration.yml
cat > /usr/share/redmine/config/database.yml <<EOF
# Default setup is given for MySQL with ruby1.9.
# Examples for PostgreSQL, SQLite3 and SQL Server can be found at the end.
# Line indentation must be 2 spaces (no tabs).

production:
  adapter: mysql2
  database: ${MYSQL_DB}
  host: localhost
  username: ${MYSQL_USER}
  password: ${MYSQL_PASS}
  encoding: utf8

development:
  adapter: mysql2
  database: ${MYSQL_DB}_development
  host: localhost
  username: ${MYSQL_USER}
  password: ""
  encoding: utf8

# Warning: The database defined as "test" will be erased and
# re-generated from your development database when you run "rake".
# Do not set this db to the same as development or production.
test:
  adapter: mysql2
  database: redmine_test
  host: localhost
  username: root
  password: ""
  encoding: utf8
EOF

cat > /root/default_passwords.txt <<EOF
MYSQL_PASS=${MYSQL_PASS}
MYSQL_ROOT_PASS=${MYSQL_ROOT_PASS}
MYSQL_USER=${MYSQL_USER}
MYSQL_DB=${MYSQL_DB}
EOF

cd /usr/share/redmine
gem install bundler
gem install mysql2 puma

bundle install --without development test
bundle exec rake generate_secret_token
RAILS_ENV=production bundle exec rake db:migrate
RAILS_ENV=production REDMINE_LANG=en bundle exec rake redmine:load_default_data

#chgrp rvm /usr/share/ruby-projects/
#chmod 0775 /usr/share/ruby-projects/

run_print_credentials

chmod 600 /root/default_passwords.txt

YC_WELCOME_BODY=`cat <<EOF
You can view generated credentials in /root/default_passwords.txt

${YC_WELCOME_BODY}
EOF`

print_msg;

chown -R www-data /usr/share/redmine/
systemctl enable nginx
systemctl restart nginx

echo "yc-setup: Done"

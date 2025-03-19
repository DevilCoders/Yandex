#!/bin/bash

set -e

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-clean.sh

HOME=/root

systemd-run --property="After=apt-daily.service apt-daily-upgrade.service" --wait /bin/true
echo "Install packages ..."

apt-add-repository -y ppa:rael-gc/rvm
apt-get -y update

mkdir -p /usr/share/ruby-projects

curl -sL https://dl.yarnpkg.com/debian/pubkey.gpg | apt-key add -
echo "deb https://dl.yarnpkg.com/debian/ stable main" > /etc/apt/sources.list.d/yarn.list

apt-get -y update && sudo apt-get install -y --no-install-recommends yarn

curl -sL https://deb.nodesource.com/setup_10.x | sudo bash -
apt-get -y update && sudo apt-get install -y nodejs

#echo "Updating locales ..."
#export LC_ALL=en_US.UTF-8
apt-get -q -y install apache2 \
    build-essential libapache2-mod-passenger \
    mysql-client libmysqlclient-dev \
    zlib1g-dev \
    cron debconf-utils git \
    iptables \
    rvm gnupg2 \

MYSQL_ROOT_PASS=ZkbEsVNb2rXKT

echo iptables-persistent iptables-persistent/autosave_v4 boolean false | debconf-set-selections
echo iptables-persistent iptables-persistent/autosave_v6 boolean false | debconf-set-selections
debconf-set-selections <<< "mysql-server-5.7 mysql-server/root_password password ${MYSQL_ROOT_PASS}"
debconf-set-selections <<< "mysql-server-5.7 mysql-server/root_password_again password ${MYSQL_ROOT_PASS}"

apt-get -q -y install iptables-persistent mysql-server-5.7

echo "Replacing adduser configs"
sed -i "s/#ADD_EXTRA_GROUPS=1/ADD_EXTRA_GROUPS=1/" /etc/adduser.conf
sed -i '/#EXTRA_GROUPS/c\EXTRA_GROUPS="nvm rvm"' /etc/adduser.conf

#usermod -a -G rvm www-data
usermod -a -G rvm root

cat >> /etc/bash.bashrc << EOF
# Load RVM into a shell session *as a function*
source "/etc/profile.d/rvm.sh"
EOF

source /etc/profile.d/rvm.sh
gpg2 --homedir /root/.gnupg --recv-keys 409B6B1796C275462A1703113804BB82D39DC0E3
gpg2 --homedir /root/.gnupg --recv-keys 7D2BAF1CF37B13E2069D6956105BD0E739499BDB
curl -sSL https://rvm.io/mpapis.asc | gpg --homedir /root/.gnupg --import -

echo "Updating RVM"
#rvm get stable
#rvm get stable --auto-dotfiles
rvm install 2.6.3
echo "Installing Ruby"
rvm install ruby
echo "Installing Bundler"
gem install bundler
gem install mysql2 puma
gem install rails
gem install sass-rails

echo "Setup apache2 ..."

cat > /etc/apache2/sites-enabled/000-default.conf <<EOF
<VirtualHost *:80 [::]:80>
        ServerAdmin webmaster@localhost
        DocumentRoot /usr/share/ruby-projects/testapp/public

        ErrorLog \${APACHE_LOG_DIR}/error.log
        CustomLog \${APACHE_LOG_DIR}/access.log combined

        RailsEnv development
        PassengerRuby /usr/share/rvm/wrappers/ruby-2.6.3/ruby

        <Directory "/usr/share/ruby-projects/testapp/public">
            Options FollowSymLinks
            Require all granted
        </Directory>

</VirtualHost>

EOF

systemctl disable apache2
echo "Replacing adduser configs"
sed -i "s/#ADD_EXTRA_GROUPS=1/ADD_EXTRA_GROUPS=1/" /etc/adduser.conf
sed -i '/#EXTRA_GROUPS/c\EXTRA_GROUPS="rvm"' /etc/adduser.conf
#TODO: Don't forget about useradd command. Add users to default RVM group on user creation

chown -R www-data /usr/share/ruby-projects/

generate_print_credentials;
on_install;
print_msg;


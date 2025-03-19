#!/bin/bash

set -e

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-clean.sh
export DEBIAN_FRONTEND=noninteractive

HOME=/root

echo "Install packages ..."

systemd-run --property="After=apt-daily.service apt-daily-upgrade.service" --wait /bin/true

apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 561F9B9CAC40B2F7
apt-get install -y apt-transport-https ca-certificates

apt-add-repository -y ppa:rael-gc/rvm
sh -c 'echo deb https://oss-binaries.phusionpassenger.com/apt/passenger bionic main > /etc/apt/sources.list.d/passenger.list'
apt-get -y update

mkdir -p /usr/share/redmine

#echo "Updating locales ..."
#export LC_ALL=en_US.UTF-8
apt-get -q -y install  \
    build-essential \
    libmysqlclient-dev \
    zlib1g-dev nodejs \
    cron debconf-utils \
    iptables \
    rvm gnupg2 dirmngr \
    nginx-extras passenger libnginx-mod-http-passenger \
    unzip imagemagick libmagickwand-dev

MYSQL_ROOT_PASS=ZkbEsVNb2rXKT

echo iptables-persistent iptables-persistent/autosave_v4 boolean false | debconf-set-selections
echo iptables-persistent iptables-persistent/autosave_v6 boolean false | debconf-set-selections
debconf-set-selections <<< "mariadb-server-10.0 mysql-server/root_password password ${MYSQL_ROOT_PASS}"
debconf-set-selections <<< "mariadb-server-10.0 mysql-server/root_password_again password ${MYSQL_ROOT_PASS}"

apt-get -q -y install iptables-persistent
apt-get -q -y install mariadb-server

usermod -a -G rvm root

cat >> /etc/bash.bashrc << EOF
# Load RVM into a shell session *as a function*
source "/etc/profile.d/rvm.sh"
EOF

source /etc/profile.d/rvm.sh
curl -sSL https://rvm.io/mpapis.asc | gpg2 --homedir /root/.gnupg --import -
curl -sSL https://rvm.io/pkuczynski.asc | gpg2 --homedir /root/.gnupg --import -


echo "Updating RVM"
#rvm get stable
rvm get stable --auto-dotfiles
echo "Installing Ruby"
rvm install ruby-2.6.3
echo "Installing Bundler"
#gem install rails

echo "Downloading Redmine"
wget --retry-connrefused --waitretry=1 --read-timeout=20 --timeout=15 -t 3 https://www.redmine.org/releases/redmine-4.1.0.zip -O /tmp/redmine.zip
unzip /tmp/redmine.zip -d /tmp/
cp -r /tmp/redmine-4.1.0/* /usr/share/redmine
rm -rf /tmp/redmine.zip
rm -rf /tmp/redmine-4.1.0

echo "Setup nginx ..."
cat > /etc/nginx/sites-available/redmine.conf <<EOF
server
{
	listen 80 default;
	root /usr/share/redmine/public/;
	passenger_enabled on;
	passenger_ruby /usr/share/rvm/gems/ruby-2.6.3/wrappers/ruby;
}
EOF

rm -f /etc/nginx/sites-enabled/default
ln -s /etc/nginx/sites-available/redmine.conf /etc/nginx/sites-enabled

sed -i "s@# include /etc/nginx/passenger.conf;@include /etc/nginx/passenger.conf;@" /etc/nginx/nginx.conf

systemctl disable nginx
echo "Replacing adduser configs"
sed -i "s/#ADD_EXTRA_GROUPS=1/ADD_EXTRA_GROUPS=1/" /etc/adduser.conf
sed -i '/#EXTRA_GROUPS/c\EXTRA_GROUPS="rvm"' /etc/adduser.conf
#TODO: Don't forget about useradd command. Add users to default RVM group on user creation

chown -R www-data /usr/share/redmine/

generate_print_credentials;
on_install;
print_msg;

#!/bin/bash

set -ev

source /opt/yc-marketplace/yc-clean.sh

export DEBIAN_FRONTEND=noninteractive
HOME=/root

echo iptables-persistent iptables-persistent/autosave_v4 boolean false | debconf-set-selections
echo iptables-persistent iptables-persistent/autosave_v6 boolean false | debconf-set-selections

systemd-run --property="After=apt-daily.service apt-daily-upgrade.service" --wait /bin/true

apt-get -y update
apt-get -y install curl git lsb-release apt-transport-https build-essential openssl libssl-dev pkg-config g++ iptables iptables-persistent make python

#add-apt-repository -y -r ppa:chris-lea/node.js || echo "No Repo"
#rm -f /etc/apt/sources.list.d/chris-lea-node_js-${DISTRO}.list

#curl -s https://deb.nodesource.com/gpgkey/nodesource.gpg.key | apt-key add -

#echo "deb https://deb.nodesource.com/${NODEREPO} ${DISTRO} main" > /etc/apt/sources.list.d/nodesource.list
#echo "deb-src https://deb.nodesource.com/${NODEREPO} ${DISTRO} main" >> /etc/apt/sources.list.d/nodesource.list

#apt-get update && apt-get install -y make nodejs

curl -sL https://dl.yarnpkg.com/debian/pubkey.gpg | apt-key add -
echo "deb https://dl.yarnpkg.com/debian/ stable main" > /etc/apt/sources.list.d/yarn.list

apt-get update && sudo apt-get install -y --no-install-recommends yarn

groupadd nvm
usermod -aG nvm root
mkdir /usr/local/nvm
chown :nvm /usr/local/nvm
chmod -R g+ws /usr/local/nvm

git clone https://github.com/creationix/nvm.git /opt/nvm

cat >> /root/.bashrc <<EOF

export NVM_DIR=/usr/local/nvm
source /opt/nvm/nvm.sh
EOF

export NVM_DIR=/usr/local/nvm
source /opt/nvm/nvm.sh

nvm install 12
#End

cat > /etc/profile.d/nvm.sh <<EOF
#!/bin/bash
export NVM_DIR="/usr/local/nvm"
[ -s "/opt/nvm/nvm.sh" ] && . "/opt/nvm/nvm.sh"
EOF

chmod +x /etc/profile.d/nvm.sh

cat >> /etc/bash.bashrc <<EOF

[ -s "$NVM_DIR/bash_completion" ] && \. "$NVM_DIR/bash_completion"
EOF

echo "Replacing adduser configs"
sed -i "s/#ADD_EXTRA_GROUPS=1/ADD_EXTRA_GROUPS=1/" /etc/adduser.conf
sed -i '/#EXTRA_GROUPS/c\EXTRA_GROUPS="nvm"' /etc/adduser.conf


on_install;


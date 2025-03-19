#!/bin/bash
set -ev

echo "Add dhcp6 ..."

# Fix the issue that localhost doesn't resolve into ::1
sed -i "s/\(^.*ip6-loopback\).*/\1 localhost/" /etc/hosts

# Use DHCPv6 on eth0 interface
cat > /etc/cloud/cloud.cfg.d/10-networking.cfg << 'EOF'
# Use DHCPv6 with DHCPv4 on eth0
network:
  version: 1
  config:
  - type: physical
    name: eth0
    subnets:
      - type: dhcp
      - type: dhcp6
  - type: physical
    name: eth1
    subnets:
      - type: dhcp
EOF

# Disable journald rate limits to avoid loss of any logs
cat >> /etc/systemd/journald.conf << 'EOF'

RateLimitInterval = 0
RateLimitBurst = 0
Storage=persistent
EOF

# Add NoCloud Datasource to default datasources
cat > /etc/cloud/cloud.cfg.d/90_dpkg.cfg << 'EOF'
# to update this file, run dpkg-reconfigure cloud-init
datasource_list: [ NoCloud, Ec2 ]
manage_etc_hosts: true
EOF

cat > /etc/apt/sources.list.d/dist-yandex.list <<EOF
deb http://yandex-cloud.dist.yandex.ru/yandex-cloud stable/all/
deb http://yandex-cloud.dist.yandex.ru/yandex-cloud stable/amd64/
deb http://yandex-cloud.dist.yandex.ru/yandex-cloud testing/all/
deb http://yandex-cloud.dist.yandex.ru/yandex-cloud testing/amd64/
deb http://yandex-cloud.dist.yandex.ru/yandex-cloud unstable/all/
deb http://yandex-cloud.dist.yandex.ru/yandex-cloud unstable/amd64/
deb http://dist.yandex.ru/common/ stable/all/
deb http://dist.yandex.ru/common/ stable/amd64/
EOF

#curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
curl http://dist.yandex.ru/REPO.asc | sudo apt-key add -

apt-get -y update
apt-get -y install apt-transport-https
apt-add-repository https://mirror.yandex.ru/ubuntu
#add-apt-repository \
#   "deb [arch=amd64] https://download.docker.com/linux/ubuntu \
#   $(lsb_release -cs) \
#   stable"

apt-get -y update 
apt-get -q -y install python
#apt-get -q -y install docker-ce

echo iptables-persistent iptables-persistent/autosave_v4 boolean false | debconf-set-selections
echo iptables-persistent iptables-persistent/autosave_v6 boolean false | debconf-set-selections

apt-get -q -y install iptables-persistent

if [[ `grep -e '^PermitRootLogin' /etc/ssh/sshd_config` ]];
then sed -i 's/PermitRootLogin.*/PermitRootLogin no/g' /etc/ssh/sshd_config
else echo "PermitRootLogin no" >> /etc/ssh/sshd_config
fi

if [[ `grep -e '^PasswordAuthentication' /etc/ssh/sshd_config` ]];
then sed -i 's/PasswordAuthentication.*/PasswordAuthentication no/g' /etc/ssh/sshd_config;
else echo "PasswordAuthentication no" >> /etc/ssh/sshd_config;
fi

echo "auto eth0" >> /etc/network/interfaces
echo "iface eth0 inet dhcp" >> /etc/network/interfaces
echo "iface eth0 inet6 dhcp" >> /etc/network/interfaces

#sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT=.*/GRUB_CMDLINE_LINUX_DEFAULT=""/g' /etc/default/grub; 

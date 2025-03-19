#!/bin/bash -e

apt-get -y update
apt-get -y install cloud-init curl vim
apt-get -y upgrade
apt-get -y install apt-transport-https

sed -i "s@#FallbackNTP.*@FallbackNTP=ntp0.NL.net clock.isc.org ntp2.vniiftri.ru ntps1-1.cs.tu-berlin.de ntp.ix.ru@" /etc/systemd/timesyncd.conf

echo 'datasource_list: [ NoCloud, GCE, Ec2, None ]' | tee /etc/cloud/cloud.cfg.d/90_dpkg.cfg
cat > /etc/cloud/cloud.cfg.d/00_Ec2.cfg <<EOF
datasource:
 Ec2:
  strict_id: false
EOF

cat > /etc/cloud/cloud.cfg.d/95-yandex-cloud.cfg <<EOF
# Yandex.Cloud repo configuration
manage_etc_hosts: true

system_info:
   package_mirrors:
     - arches: [i386, amd64]
       failsafe:
         primary: http://archive.ubuntu.com/ubuntu
         security: http://security.ubuntu.com/ubuntu
       search:
         primary:
           - http://mirror.yandex.ru/ubuntu/
         security: []
     - arches: [armhf, armel, default]
       failsafe:
         primary: http://ports.ubuntu.com/ubuntu-ports
         security: http://ports.ubuntu.com/ubuntu-ports
EOF

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
EOF

# Disable journald rate limits to avoid loss of any logs
cat >> /etc/systemd/journald.conf << 'EOF'

RateLimitInterval = 0
RateLimitBurst = 0
Storage=persistent
EOF

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


#!/bin/bash

set -ev

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-webauth.sh


echo "Setup start"

rm $0;
rm /etc/cron.d/first-boot

flush_welcome_body

/usr/local/openvpn_as/bin/ovpn-init --host $(curl http://169.254.169.254/latest/meta-data/public-ipv4) --batch --force > /tmp/output
sed -n '/Initial Configuration Complete/,$p' -i /tmp/output
INSTALLER_TEXT=$(</tmp/output)

OPENVPN_PASS=`head /dev/urandom | tr -dc A-Za-z0-9 | head -c 13`

echo "openvpn:${OPENVPN_PASS}" | chpasswd openvpn

systemctl enable openvpnas
systemctl start openvpnas

netfilter-persistent save

run_print_credentials

cat > /root/default_passwords.txt <<EOF
OPENVPN_USER=openvpn
OPENVPN_PASS=${OPENVPN_PASS}

${INSTALLER_TEXT}
EOF

rm -f /tmp/output

YC_WELCOME_BODY=`cat <<EOF
You can view generated credentials in /root/default_passwords.txt

${YC_WELCOME_BODY}
EOF`

print_msg;
cat /root/default_passwords.txt > /dev/ttyS0

echo "yc-setup: Done"

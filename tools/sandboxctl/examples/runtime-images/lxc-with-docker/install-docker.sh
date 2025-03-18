#!/bin/bash

apt-get install yandex-internal-root-ca
apt-get --yes purge yandex-search-common-apt

apt-get update
apt-get --yes install curl git python-pip software-properties-common iptables-persistent
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | apt-key add -
add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu bionic stable"
apt-get --yes install docker-ce

cat >> /etc/environment << EOF
REQUESTS_CA_BUNDLE="/etc/ssl/certs/ca-certificates.crt"
EOF

mkdir -p /etc/iptables
cat > /etc/iptables/rules.v6 << EOF
*filter
:INPUT ACCEPT [176:22921]
:FORWARD ACCEPT [0:0]
:OUTPUT ACCEPT [29:5356]
COMMIT
*nat
:PREROUTING ACCEPT [101:8396]
:INPUT ACCEPT [10:720]
:OUTPUT ACCEPT [80:6923]
:POSTROUTING ACCEPT [80:6923]
-A POSTROUTING -s fd00::/8 -j MASQUERADE
COMMIT
EOF

mkdir -p /etc/docker
cat > /etc/docker/daemon.json << EOF
{
	"ipv6": true,
	"fixed-cidr-v6": "fd00::/8",
	"ip-forward": true
}
EOF
usermod -a -G docker sandbox

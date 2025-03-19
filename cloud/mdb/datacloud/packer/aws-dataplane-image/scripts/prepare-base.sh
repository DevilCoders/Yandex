#!/bin/bash

set -x
set -e

if [ $# -lt 2 ]; then
  echo "usage: $0 <deploy-api-hostname> <vm-image-hostname>" 1>&2
  exit 1
fi

DEPLOY_API_HOST=$1
IMAGE_HOSTNAME=$2
ARCHITECTURE=`uname -m`
if [ ${ARCHITECTURE} == 'x86_64' ]; then
  ARCH='amd64'
  GRUB_PACKAGE='grub-pc'
elif [ ${ARCHITECTURE} == 'aarch64' ]; then
  ARCH='arm64'
  GRUB_PACKAGE='grub-efi-arm64'
else
  echo "Unsupported architecture"
  exit 1
fi

apt-get -qq update
apt-get -y upgrade
apt-get -qq update

apt-get purge unattended-upgrades -y

apt-get -y install awscli
aws s3 cp s3://mdb-bionic-stable-${ARCH}/pool/main/a/apt-golang-s3/apt-golang-s3_2.52-3017fcc_${ARCH}.deb /tmp/apt-golang-s3.deb
dpkg -i /tmp/apt-golang-s3.deb
apt-key add - </tmp/mdb-bionic.gpg
cat <<EOF >/etc/apt/sources.list.d/datacloud.list
deb [arch=${ARCH}] s3://mdb-bionic-stable-${ARCH} .- main
deb [arch=all] s3://mdb-bionic-stable-all .- main
EOF
echo "Acquire::s3::region eu-central-1;" >/etc/apt/apt.conf.d/s3

locale-gen en_US.UTF-8
apt-get -qq update
export LANG=C
export DEBIAN_FRONTEND=noninteractive
apt-get -y install yandex-archive-keyring
apt-get -y install ${GRUB_PACKAGE}
apt-get -y install virt-what tcpdump
apt-get -y install \
  salt-common=3004+ds-1+yandex0 \
  salt-minion=3004+ds-1+yandex0 \
  python3-msgpack=0.5.6-1 \
  mdb-config-salt=1.9557427-trunk \
  yandex-search-user-monitor=1.0-45 \
  python3-boto \
  python3-botocore \
  mdb-ssh-keys \
  parted \
  yandex-default-locale-en

apt-get -y --force-yes install mdadm
systemctl disable \
    systemd-networkd.socket \
    systemd-networkd \
    systemd-timesyncd \
    systemd-resolved \
    networkd-dispatcher \
    systemd-networkd-wait-online
systemctl mask \
    systemd-networkd.socket \
    systemd-networkd \
    systemd-timesyncd \
    systemd-resolved \
    networkd-dispatcher \
    systemd-networkd-wait-online
apt-get -y install ifupdown
apt-get -y purge nplan netplan.io
rm -f "/etc/salt/minion_id"
rm -f "/etc/dhcp/dhclient-enter-hooks.d/resolved"

cat <<EOF > "${MNT_DIR}/etc/network/interfaces"
auto lo
iface lo inet loopback

auto ens5
iface ens5 inet dhcp
iface ens5 inet6 dhcp
EOF

mkdir -p /etc/yandex/mdb-deploy
echo 2 >/etc/yandex/mdb-deploy/deploy_version
echo "$DEPLOY_API_HOST" >/etc/yandex/mdb-deploy/mdb_deploy_api_host
echo $IMAGE_HOSTNAME >/etc/salt/minion_id
mkdir -p "/etc/cloud/cloud.cfg.d"
mv /tmp/master-sign.pub /etc/salt/pki/minion/master_sign.pub
mkdir -p /opt/yandex
mv /tmp/allCAs.pem /opt/yandex/allCAs.pem

hostname $IMAGE_HOSTNAME
mv -f /etc/hosts /etc/hosts.bak
cat <<EOF >/etc/hosts
127.0.0.1  $IMAGE_HOSTNAME
127.0.0.1  localhost
::1  $IMAGE_HOSTNAME
::1 ip6-localhost ip6-loopback
EOF

curl -s --show-error --fail --cacert /opt/yandex/allCAs.pem -X PUT -H 'Content-Type: application/json' -d "{\"fqdn\":\"$IMAGE_HOSTNAME\",\"autoReassign\":true,\"group\":\"eu-central-1\"}" "https://$DEPLOY_API_HOST/v1/minions/$IMAGE_HOSTNAME"
curl -s --show-error --fail --cacert /opt/yandex/allCAs.pem -X POST "https://$DEPLOY_API_HOST/v1/minions/$IMAGE_HOSTNAME/unregister"
timeout 600 bash -c "while ! salt-call get_master.master; do sleep 1; done"

echo "SUCCESS!"
exit 0

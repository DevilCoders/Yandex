#!/bin/bash

set -e

container=$1
if [ -z "$container" ]; then
    echo "Container name is mandatory."
    exit 1
fi
path=`portoctl get ${container} root`

if [ -d "${path}/bin" ]; then
    echo "${path} is not empty."
    exit 1
fi

tar xjf /data/images/bionic.tar.bz2 --numeric-owner -C ${path}

echo "deb http://mirror.yandex.ru/ubuntu bionic main restricted universe multiverse" >${path}/etc/apt/sources.list
echo "deb http://mirror.yandex.ru/ubuntu bionic-security main restricted universe multiverse" >>${path}/etc/apt/sources.list
echo "deb http://mirror.yandex.ru/ubuntu bionic-updates main restricted universe multiverse" >>${path}/etc/apt/sources.list
portoctl exec bootstrap-${container}-packages command="apt-get -qq update" root="${path}"
portoctl exec bootstrap-${container}-upgrade command="apt-get -y -o Dpkg::Options::='--force-confdef' -o Dpkg::Options::='--force-confnew' upgrade" \
                        root="${path}" env="DEBIAN_FRONTEND=noninteractive"
cat <<EOF >> ${path}/etc/apt/sources.list.d/mdb-bionic-stable.list
deb http://dist.yandex.ru/mdb-bionic stable/all/
deb http://dist.yandex.ru/mdb-bionic stable/\$(ARCH)/
EOF

cat <<EOF >> ${path}/etc/apt/sources.list.d/mdb-bionic-secure-stable.list
deb http://dist.yandex.ru/mdb-bionic-secure stable/all/
deb http://dist.yandex.ru/mdb-bionic-secure stable/\$(ARCH)/
EOF

mkdir -p ${path}/opt/yandex/
cp /opt/yandex/allCAs.pem ${path}/opt/yandex/

portoctl exec bootstrap-${container}-locale command="locale-gen en_US.UTF-8" root="${path}"
portoctl exec bootstrap-${container}-packages command="apt-get -qq update" root="${path}"
portoctl exec bootstrap-${container}-packages command="apt-get -y install yandex-archive-keyring" root="${path}"
portoctl exec bootstrap-${container}-packages command="apt-get -qq update" root="${path}"
portoctl exec bootstrap-${container}-packages command="apt-get -o Dpkg::Options::='--force-confdef' -o Dpkg::Options::='--force-confnew' -y install \
                           salt-common=3000.9+ds-1+yandex0 \
                           salt-minion=3000.9+ds-1+yandex0 \
                           mdb-config-salt=1.8748427-trunk \
                           yandex-selfdns-client \
                           mdb-ssh-keys \
                           virt-what \
                           tcpdump \
                           config-caching-dns=1.0-12 \
                           yandex-default-locale-en" root="${path}" resolv_conf=""
portoctl exec bootstrap-${container}-packages command="systemctl enable mdb-ping-salt-master" root="${path}"

if [ -f "${path}/lib/init/fstab" ]; then
    sed -i ${path}/lib/init/fstab -e 's/^/#/'
fi

rsync -a /etc/yandex/selfdns-client/default.conf ${path}/etc/yandex/selfdns-client/default.conf
portoctl exec bootstrap-${container}-selfdns command="chown root:selfdns /etc/yandex/selfdns-client/default.conf" root="${path}"
hostname -f >${path}/etc/dom0hostname
echo ${container} > ${path}/etc/salt/minion_id

echo "net.ipv6.conf.all.forwarding = 1" >${path}/etc/sysctl.d/30-net-porto.conf
echo "net.ipv6.conf.all.proxy_ndp = 1" >>${path}/etc/sysctl.d/30-net-porto.conf

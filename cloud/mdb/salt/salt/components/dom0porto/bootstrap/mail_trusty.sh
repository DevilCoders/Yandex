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

tar xjf /data/images/trusty.tar.bz2 --numeric-owner -C ${path}

echo "deb http://mirror.yandex.ru/ubuntu trusty main restricted universe multiverse" >${path}/etc/apt/sources.list
echo "deb http://mirror.yandex.ru/ubuntu trusty-security main restricted universe multiverse" >>${path}/etc/apt/sources.list
echo "deb http://mirror.yandex.ru/ubuntu trusty-updates main restricted universe multiverse" >>${path}/etc/apt/sources.list
portoctl exec bootstrap-${container}-packages command="apt-get -qq update" root="${path}"
portoctl exec bootstrap-${container}-upgrade command="apt-get -y -o Dpkg::Options::='--force-confdef' -o Dpkg::Options::='--force-confnew' upgrade" \
                        root="${path}" env="DEBIAN_FRONTEND=noninteractive"
echo "deb http://dist.yandex.ru/common stable/\$(ARCH)/" >${path}/etc/apt/sources.list.d/common-stable.list
echo "deb http://dist.yandex.ru/common stable/all/" >>${path}/etc/apt/sources.list.d/common-stable.list
echo "deb http://dist.yandex.ru/mail-trusty stable/\$(ARCH)/" >${path}/etc/apt/sources.list.d/mail-trusty-stable.list
echo "deb http://dist.yandex.ru/mail-trusty stable/all/" >>${path}/etc/apt/sources.list.d/mail-trusty-stable.list
echo "deb http://dist.yandex.ru/yandex-trusty stable/\$(ARCH)/" >${path}/etc/apt/sources.list.d/yandex-trusty-stable.list
echo "deb http://dist.yandex.ru/yandex-trusty stable/all/" >>${path}/etc/apt/sources.list.d/yandex-trusty-stable.list
portoctl exec bootstrap-${container}-locale command="locale-gen en_US.UTF-8" root="${path}"
portoctl exec bootstrap-${container}-packages command="apt-get -qq update" root="${path}"
portoctl exec bootstrap-${container}-packages command="apt-get -y --force-yes install yandex-archive-keyring" root="${path}"
portoctl exec bootstrap-${container}-packages command="apt-get -qq update" root="${path}"
portoctl exec bootstrap-${container}-packages command="apt-get -o Dpkg::Options::='--force-confdef' -o Dpkg::Options::='--force-confnew' -y install \
                           salt-common=2018.3.3+ds-2+yandex0 \
                           salt-minion=2018.3.3+ds-2+yandex0 \
                           mdb-config-salt=1.7418792-trunk \
                           yandex-selfdns-client \
                           mdb-ssh-keys \
                           virt-what \
                           tcpdump \
                           config-caching-dns=1.0-12 \
                           yandex-default-locale-en" root="${path}" resolv_conf=""

if [ -f "${path}/lib/init/fstab" ]; then
    sed -i ${path}/lib/init/fstab -e 's/^/#/'
fi

rsync -a /etc/yandex/selfdns-client/default.conf ${path}/etc/yandex/selfdns-client/default.conf
portoctl exec bootstrap-${container}-selfdns command="chown root:selfdns /etc/yandex/selfdns-client/default.conf" root="${path}"
hostname -f >${path}/etc/dom0hostname
echo ${container} > ${path}/etc/salt/minion_id

echo "net.ipv6.conf.all.forwarding = 1" >${path}/etc/sysctl.d/30-net-porto.conf
echo "net.ipv6.conf.all.proxy_ndp = 1" >>${path}/etc/sysctl.d/30-net-porto.conf

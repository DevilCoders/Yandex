#!/usr/bin/env bash

set -eux

configure-base() {
    # Set super-secret root password for debug purpose
    sed -i -e 's|root.*|root:$6$EBXz/enV$GbZRvE2ObGvoM44g0YN7xe8FQp1E2UQ2vCE8TiAx8INsgz1MI/lBetYKnqnnWJo12qwdd0o28QmT34zyZwAJL/:17863:0:99999:7:::|'  /etc/shadow

    # Fix the issue that localhost doesn't resolve into ::1
    sed -i "s/\(^.*ip6-loopback\).*/\1 localhost/" /etc/hosts

    # Force using EC2 Metadata API even if the instance is running under cloud provider different from Amazon EC2
    cat > /etc/cloud/cloud.cfg.d/99-ec2-datasource.cfg << 'EOF'
#cloud-config
datasource:
 Ec2:
  strict_id: false
EOF

    # Add NoCloud Datasource to default datasources
    cat > /etc/cloud/cloud.cfg.d/90_dpkg.cfg << 'EOF'
# to update this file, run dpkg-reconfigure cloud-init
datasource_list: [ NoCloud, Ec2 ]
EOF
}

configure-network() {
    rm /etc/network/interfaces.d/*

    cat > /etc/network/interfaces << 'EOF'
auto lo
iface lo inet loopback

auto eth0
iface eth0 inet dhcp
iface eth0 inet6 dhcp
  # default route based on RA
  accept_ra 2
  # will not fallback to SLAAC
  # below used in /lib/ifupdown/wait-for-ll6.sh
  # to wait ready state for NIC to assign address from DHCP
  ll-attempts 120
  ll-interval 0.5
EOF

    cat > /etc/sysctl.d/30-network.conf << 'EOF'
# do not use DAD (speed up assign of link-local address)
net.ipv6.conf.eth0.accept_dad = 0
net.ipv6.conf.eth0.dad_transmits = 0
EOF

    # Disable cloud-init networking. Use yc-network-config instead
    cat > /etc/cloud/cloud.cfg.d/99-disable-network-config.cfg << 'EOF'
network:
 config:
  disabled
EOF

# TODO: add timeout for dhcp client

}

configure-repositories() {
    # Disable default repositories
    sed -i -e 's/^deb/#deb/' /etc/apt/sources.list

    cat > /etc/apt/sources.list.d/yandex.list << 'EOF'
deb http://yandex-xenial.dist.yandex.ru/yandex-xenial stable/all/
deb http://yandex-xenial.dist.yandex.ru/yandex-xenial stable/$(ARCH)/
EOF

    cat > /etc/apt/sources.list.d/yandex-cloud-stable.list << 'EOF'
deb http://yandex-cloud.dist.yandex.ru/yandex-cloud stable/all/
EOF

    cat > /etc/apt/sources.list.d/yandex-cloud-amd64-stable.list << 'EOF'
deb http://yandex-cloud.dist.yandex.ru/yandex-cloud stable/amd64/
EOF

    cat > /etc/apt/sources.list.d/mirror-yandex.list << 'EOF'
deb http://mirror.yandex.ru/ubuntu xenial main restricted
deb-src http://mirror.yandex.ru/ubuntu xenial main restricted
deb http://mirror.yandex.ru/ubuntu xenial-updates main restricted
deb-src http://mirror.yandex.ru/ubuntu xenial-updates main restricted
deb http://mirror.yandex.ru/ubuntu xenial universe
deb-src http://mirror.yandex.ru/ubuntu xenial universe
deb http://mirror.yandex.ru/ubuntu xenial-updates universe
deb-src http://mirror.yandex.ru/ubuntu xenial-updates universe
deb http://mirror.yandex.ru/ubuntu xenial multiverse
deb-src http://mirror.yandex.ru/ubuntu xenial multiverse
deb http://mirror.yandex.ru/ubuntu xenial-updates multiverse
deb-src http://mirror.yandex.ru/ubuntu xenial-updates multiverse
deb http://mirror.yandex.ru/ubuntu xenial-backports main restricted universe multiverse
deb-src http://mirror.yandex.ru/ubuntu xenial-backports main restricted universe multiverse
deb http://mirror.yandex.ru/ubuntu xenial-security main restricted universe multiverse
deb-src http://mirror.yandex.ru/ubuntu xenial-security main restricted universe multiverse
EOF

    apt-get update
    apt-get install -y --allow-unauthenticated yandex-archive-keyring
}

upgrade-packages() {
    apt-get upgrade -y --allow-unauthenticated --option=Dpkg::Options::=--force-confold

#    #We need to overwrite network config.
#    apt-get -o DPkg::options::=--force-confmiss -o DPkg::options::=--force-confnew  install yc-network-config
}

cleanup-system() {
    # Set correct boot drive in grub
#    sed -i -e "s/$NBD_DEVICEp1/vda1/" /boot/grub/grub.cfg

    # Clear apt caches
    apt-get clean all
    rm /var/lib/apt/lists/* || :

    rm -rf /var/lib/cloud
    rm -f /var/lib/dhcp/*.leases
}

export DEBIAN_FRONTEND=noninteractive

configure-base
configure-network
configure-repositories
upgrade-packages
cleanup-system

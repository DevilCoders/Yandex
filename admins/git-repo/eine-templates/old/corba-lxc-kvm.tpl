%r:vlan-switch! 141

%r:pxe-switch! -p eine
%discovery smart=relax mem=relax

%^setup 
clearpart --all

part raid.00 -d disk:0 -s 20G
part raid.10 -d disk:0 -s 5G
part raid.20 -d disk:0 --grow

part raid.01 -d disk:1 -s 20G
part raid.11 -d disk:1 -s 5G
part raid.21 -d disk:1 --grow

part raid.02 -d disk:2 -s 20G
part raid.12 -d disk:2 -s 5G
part raid.22 -d disk:2 --grow

part raid.03 -d disk:3 -s 20G
part raid.13 -d disk:3 -s 5G
part raid.23 -d disk:3 --grow

raid / -d md0 -l 1 -t ext3 raid.00 raid.01 raid.02 raid.03
raid swap -d md1 -l 10 raid.10 raid.11 raid.12 raid.13
raid pv -d md2 -l 10 raid.20 raid.21 raid.22 raid.23

volgroup vg00 md2

install -d tmpfs p2p://ubuntu/10.04.2-x86_64
repo -l lucid mirror/ubuntu --tmp
repo local ubuntu hardy --tmp
repo dist/system configs/all/
repo dist/system lucid/all/
repo dist/system lucid/amd64/
repo dist/common stable/all/
repo dist/common stable/amd64/
repo dist/yandex-lucid stable/all/
repo dist/yandex-lucid stable/amd64/

packages bridge-utils
packages apt bind9 irqbalance vim ethtool ntpdate strace less sudo file atop ssh rsync build-essential mc finger netcat tcpdump mailutils whois traceroute host screen 

network -s conductor --bridged
resolv --dns 127.0.0.1 --dns 2a02:6b8::1
resolv --search yandex.net --domain yandex.net

bootloader
packages linux-image-3.0.4-2-lxcna 

packages htop dstat lvm2 parted syslog-ng
packages config-corba-admins-public-keys yandex-archive-keyring config-yabs-ntp
packages config-corba-lxc vmlist lxctl-template-ubuntu-12.04-amd64
packages config-juggler-client-media corba-juggler-virtual-meta juggler-client monrun config-monitoring-common config-monitoring-corba-ssh-check config-monitoring-corba-la-check yandex-juggler-containers-checks yandex-juggler-conntrackissmall-check
packages yandex-corba-config-sysctl

chroot /target dpkg-reconfigure openssh-server
sed -i 's/options {/options { \nforwarders { 2a02:6b8::1:1; };\n/' /target/etc/bind/named.conf.options

packages corba-eine-postinstall


%r:pxe-switch! -p -c

%reboot 

%r:vlan-switch! 639
639 .*dev.*
622 .*corba.*
629 .*tools.*

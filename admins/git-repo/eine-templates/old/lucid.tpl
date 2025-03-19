%r:vlan-switch! 542

%r:ipmi-boot! pxe
%^setup 
clearpart --all

part raid.00 -d disk:0 -s 20G
part raid.10 -d disk:0 -s 5G
part raid.20 -d disk:0 --grow

part raid.01 -d disk:1 -s 20G
part raid.11 -d disk:1 -s 5G
part raid.21 -d disk:1 --grow

raid / -d md0 -l 1 -t ext3 raid.00 raid.01 
raid swap -d md1 -l 1 raid.10 raid.11 
raid /storage -d md2 -l 1 -e 1.2 -t ext4 --mkfs "-b 4096 -E stride=16,stripe-width=32" -o "defaults,noatime,barrier=0,errors=remount-ro" raid.20 raid.21

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

network -s auto
resolv --dns 127.0.0.1
resolv --search yandex.net --domain yandex.net

bootloader

%r:ipmi-boot! disk

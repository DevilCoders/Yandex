%r:vlan-switch! 141

%r:pxe-switch! -p eine
%discovery smart=relax mem=relax

%^setup 
clearpart --all

part raid.10 -d disk:0 -s 400G
part raid.11 -d disk:1 -s 400G

part raid.20 -d disk:0 -s 20G
part raid.21 -d disk:1 -s 20G

raid /    -d md0 -l 1 -t ext3 raid.10 raid.11 -- --assume-clean
raid swap -d md1 -l 1 raid.20 raid.21 

install -d tmpfs p2p://debian/6.0.1a-x86_64

repo -l squeeze mirror/debian
repo local ubuntu hardy --tmp

packages htop dstat

network -s auto
resolv --dns localhost --search yandex.net --domain yandex.net

bootloader

%r:pxe-switch! -p -c
%reboot 

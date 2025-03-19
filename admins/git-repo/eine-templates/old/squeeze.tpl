%r:vlan-switch! 542
%r:ipmi-boot! pxe
%^setup 
clearpart --all

part raid.10 -d disk:0 -s 10G
part raid.00 -d disk:0 --grow

part raid.11 -d disk:1 -s 10G
part raid.01 -d disk:1 --grow

raid /    -d md0 -l 1 -t ext3 raid.00 raid.01
raid swap -d md1 -l 10       raid.10 raid.11

install -d tmpfs p2p://debian/6.0.1a-x86_64

add-repo -l squeeze mirror/debian
add-repo local eine hardy --tmp

packages htop dstat

bootloader

cleanup

network -s auto
resolv --dns localhost --search yandex.net --domain yandex.net

password -u root --plain qwerty

cleanup

%r:ipmi-boot! disk

%reboot 


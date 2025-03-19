%r:vlan-switch! 141
%r:pxe-switch! -p eine
%discovery --smart-relax --memory-relax 
%^setup 
grep '^hosts:' /etc/nsswitch.conf || echo 'hosts: files dns' >> /etc/nsswitch.conf

if ! tag nopart? ; then
	clearpart --gpt --all

	part bootgpt.01 -d disk:0 -s 20G
	part raid.02 -d disk:0 --grow

	part bootgpt.11 -d disk:1 -s 20G
	part raid.12 -d disk:1 --grow

	part bootgpt.21 -d disk:2 -s 20G
	part raid.22 -d disk:2 --grow

	part bootgpt.31 -d disk:3 -s 20G
	part raid.32 -d disk:3 --grow

fi

raid / -d md0 -e 1.2 -l 1 -t ext4 -o "defaults,noatime,barrier=0,errors=remount-ro" bootgpt.01 bootgpt.11 bootgpt.21 bootgpt.31 -- --assume-clean
raid /var -d md1 -e 1.2 -l 10 -a "-p f2" -t ext4 -o "defaults,noatime,nobarrier,nodelalloc" raid.02 raid.12 raid.22 raid.32 -- --assume-clean

# Ставим 12.04 и указываем наши репы
install -d tmpfs p2p://ubuntu/12.04-x86_64
repo -l precise mirror/ubuntu
repo dist/system configs/all/
repo dist/system precise/all/
repo dist/system precise/amd64/
repo dist/common stable/all/
repo dist/common stable/amd64/
repo dist/yandex-precise stable/all/
repo dist/yandex-precise stable/amd64/

packages yandex-archive-keyring apt linux-image-server openssh-server bind9 \
ethtool ntp ntpdate sudo file build-essential lvm2 mailutils parted syslog-ng \
procps mdadm

packages vim strace less sudo atop rsync mc finger netcat tcpdump whois traceroute \
host screen htop dstat sysstat iftop curl wget psmisc ssh dnsutils

packages config-ssh-banner config-yandex-friendly-bash config-caching-dns \
config-media-admins-public-keys yandex-dash2bash \
yandex-logrotate config-apt-allowunauth yandex-maps-config-sysctl

# Пакеты для мониторинга
packages config-caching-dns juggler-client config-juggler-client-media monrun \
config-monitoring-common config-monitoring-pkgver corba-juggler-virtual-meta  \
config-monrun-mtu-check config-monrun-daemon-check config-autodetect-active-eth

network -s conductor

resolv -6 --dns ns-cache.yandex.ru
resolv --search yandex.net --domain yandex.net

password --hash '$6$rounds=15000$SzgkLXn73dX/6RdI$1uZCIkIijvxcp7TdqzsVPO.HtH.Xz5Y4s2Hv7fMeHFXvQ3cxyzi9ePX4aN8guyVEgqEEqra5w6tyM1TgFG54e0'

post-install dpkg-reconfigure openssh-server || true
bootloader grub2
cleanup

echo >> /target/etc/apt/sources.list
cat /target/etc/apt/sources.list.d/*.list >> /target/etc/apt/sources.list
rm -f /target/etc/apt/sources.list.d/*.list

(sleep 120; reboot -f ) &

%r:pxe-switch! -p -c

%r:vlan-switch! 623

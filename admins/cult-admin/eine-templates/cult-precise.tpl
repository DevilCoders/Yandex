%r:vlan-switch! 542
%r:ipmi-boot! pxe
%discovery --smart-relax --memory-relax 
%^setup 
grep '^hosts:' /etc/nsswitch.conf || echo 'hosts: files dns' >> /etc/nsswitch.conf

# Если хостнейм содержит srv или dev - ставим lxc. Если нет - видимо виртуализация использоваться не будет. Не ставим.
if ! tag noauto? ; then
	if expr `hostname -s` : '.*-\(srv\|dev\)' >/dev/null; then
		tag +lxc
	else
		tag -lxc
	fi
fi

lxc=0
if tag lxc? ; then
	i-log log "lxc dom0"
	lxc=1
fi

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

# Если это не lxc машина - отаем все место в /var. Если lxc - создаем vg00, под который отдаем все место
raid / -d md0 -e 1.2 -l 1 -t ext4 -o "defaults,noatime,barrier=0,errors=remount-ro" bootgpt.01 bootgpt.11 bootgpt.21 bootgpt.31 -- --assume-clean
if [ $lxc -eq 0 ]; then
	raid /var -d md1 -e 1.2 -l 10 -a "-p f2" -t ext4 -o "defaults,noatime,nobarrier,nodelalloc" raid.02 raid.12 raid.22 raid.32 -- --assume-clean
else
	raid pv -d md1 -e 1.2 -l 10 -a "-p f2" raid.02 raid.12 raid.22 raid.32 -- --assume-clean
fi

if [ $lxc -eq 1 ]; then
	volgroup vg00 md1
fi

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

# Пакеты, которые мы хотим иметь на машине
## Системные пакеты
packages yandex-archive-keyring apt linux-image-server openssh-server bind9 \
ethtool ntp ntpdate sudo file build-essential lvm2 postfix mailutils parted syslog-ng \
procps mdadm

## Администрирование
packages vim strace less sudo atop rsync mc finger netcat tcpdump whois traceroute \
host screen htop dstat sysstat iftop curl wget psmisc ssh dnsutils

## Виртуализация
if [ $lxc -eq 1 ]; then
	packages linux-image-3.8.0-22-na lxc lxctl lxctl-plugin-conductor \
	lxctl-template-ubuntu-10.04-amd64 lxctl-template-ubuntu-12.04-amd64 lxctl-template-ubuntu-8.04-amd64 \
	yandex-conf-updatevirt-lxc config-corba-lxc \
	bridge-utils
fi

# Яндексовые пакеты, утилиты
## Основное
packages config-ssh-banner config-yandex-friendly-bash config-caching-dns \
config-media-admins-public-keys yandex-dash2bash \
yandex-logrotate config-apt-allowunauth config-postfix-media

## Графики, мониторинги
packages juggler-client config-juggler-client-media monrun config-monitoring-common \
config-monitoring-pkgver corba-juggler-virtual-meta config-monrun-mtu-check \

## Ставим флаг окружения. Если лоад, тестинг или дев - окружение тестовое, в другом случае - продакшн
if expr `hostname -f` : '.*dev.*' >/dev/null; then
	packages yandex-environment-development yandex-conf-repo-testing yandex-conf-repo-unstable
elif expr `hostname -f` : '*.tst.*' >/dev/null; then
	packages yandex-environment-testing yandex-conf-repo-testing
elif expr `hostname -f` : '.*load.*' >/dev/null; then
	packages yandex-environment-stress yandex-conf-repo-testing
else
	packages yandex-environment-production
fi

## Сеть
if [ $lxc -eq 1 ]; then
	network -s conductor --bridged
else
	network -s conductor
fi

resolv -6 --dns ns-cache.yandex.ru
resolv --search yandex.net --domain yandex.net

password --hash '$6$rounds=15000$SzgkLXn73dX/6RdI$1uZCIkIijvxcp7TdqzsVPO.HtH.Xz5Y4s2Hv7fMeHFXvQ3cxyzi9ePX4aN8guyVEgqEEqra5w6tyM1TgFG54e0'
mailer -r 'cult-root@yandex-team.ru' -h 'outbound-relay.yandex.net' postfix
ntp ntp1.yandex.net ntp2.yandex.net ntp3.yandex.net ntp4.yandex.net

post-install dpkg-reconfigure openssh-server || true

repo dist/yandex-precise prestable/all/ --tmp
repo dist/yandex-precise prestable/amd64/ --tmp

packages grub2

bootloader grub2
cleanup

echo >> /target/etc/apt/sources.list
cat /target/etc/apt/sources.list.d/*.list >> /target/etc/apt/sources.list
rm -f /target/etc/apt/sources.list.d/*.list

%r:vlan-switch! 623
623+639		cult-srv.*.tst.cult.yandex.net

1329+639	afisha-srv.*(dev|tst).afisha.yandex.net
1326+639	weather-srv.*(dev|tst).weather.yandex.net
1326+639	weather-srv.*.weather-(dev|tst).yandex.net
1357+639	kassa-srv.*(dev|tst).kassa.yandex.net
1556+639	jkp-srv.*(dev|tst).jkp.yandex.net
616+639		books-srv.*(dev|tst).books.yandex.net
1441+639	sport-srv.*.sport-(dev|tst).yandex.net
507+639		content-srv.*(dev|tst).content.yandex.net
1423            wrf-srv.*wrf-tst.yandex.net

1329+568	afisha-srv.*load.afisha.yandex.net
1326+568	weather-srv.*load.weather.yandex.net
1357+568	kassa-srv.*load.kassa.yandex.net
1556+568	jkp-srv.*load.jkp.yandex.net
507+568		content-srv.*load.content.yandex.net
616+568		books-srv.*load.books.yandex.net
1441+568	sport-srv.*.sport-load.yandex.net

1329		afisha-srv.*.afisha.yandex.net
1326		weather-srv.*.weather.yandex.net
516		wrf-srv.*.wrf.yandex.net
1357		kassa-srv.*.kassa.yandex.net
1556		jkp-srv.*.jkp.yandex.net
507		content-srv.*.content.yandex.net
616		books-srv.*.books.yandex.net
1441		sport-srv.*.sport.yandex.net

%r:ipmi-boot! disk

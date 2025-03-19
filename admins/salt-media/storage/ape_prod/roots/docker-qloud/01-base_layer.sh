TZ='Europe/Moscow'
LOCALE_VARS='LC_ALL=en_US.utf8 LANG=en_US.utf8 LANGUAGE=en_US.utf8'
export DEBIAN_FRONTEND="noninteractive"

cat > /etc/hosts << EOF
127.0.0.1       localhost
::1             localhost ip6-localhost ip6-loopback
fe00::0         ip6-localnet
ff00::0         ip6-mcastprefix
ff02::1         ip6-allnodes
ff02::2         ip6-allrouters
EOF

cat > /etc/apt/sources.list.d/ubuntu-security-trusty.list << EOF
deb http://mirror.yandex.ru/ubuntu/ trusty-security main restricted universe multiverse
deb http://mirror.yandex.ru/ubuntu/ trusty-updates main restricted universe multiverse
EOF

cat > /etc/apt/sources.list.d/yandex.list << EOF
deb http://common.dist.yandex.ru/common stable/all/
deb http://common.dist.yandex.ru/common stable/amd64/
deb http://common.dist.yandex.ru/common prestable/all/
deb http://common.dist.yandex.ru/common prestable/amd64/
deb http://common.dist.yandex.ru/common testing/all/
deb http://common.dist.yandex.ru/common testing/amd64/
EOF

cat > /etc/apt/sources.list.d/yandex-cocaine.list << EOF
#deb http://mirror.yandex.ru/mirrors/docker/ docker main
deb http://dist.yandex.ru/cocaine-v12-trusty testing/all/
deb http://dist.yandex.ru/cocaine-v12-trusty testing/amd64/
deb http://dist.yandex.ru/cocaine-v12-trusty prestable/all/
deb http://dist.yandex.ru/cocaine-v12-trusty prestable/amd64/
deb http://dist.yandex.ru/cocaine-v12-trusty stable/all/
deb http://dist.yandex.ru/cocaine-v12-trusty stable/amd64/
deb http://dist.yandex.ru/yandex-trusty/ stable/all/
deb http://dist.yandex.ru/yandex-trusty/ stable/amd64/
deb http://dist.yandex.ru/yandex-trusty/ prestable/all/
deb http://dist.yandex.ru/yandex-trusty/ prestable/amd64/
deb http://dist.yandex.ru/yandex-trusty/ testing/all/
deb http://dist.yandex.ru/yandex-trusty/ testing/amd64/
EOF

cat > /etc/apt/sources.list.d/yandex-storage.list << EOF
deb http://dist.yandex.ru/storage-trusty prestable/all/
deb http://dist.yandex.ru/storage-trusty prestable/amd64/
deb http://dist.yandex.ru/storage-common prestable/all/
deb http://dist.yandex.ru/storage-common prestable/amd64/
deb http://dist.yandex.ru/storage-trusty stable/all/
deb http://dist.yandex.ru/storage-trusty stable/amd64/
deb http://dist.yandex.ru/storage-common stable/all/
deb http://dist.yandex.ru/storage-common stable/amd64/
deb http://dist.yandex.ru/storage-trusty testing/all/
deb http://dist.yandex.ru/storage-trusty testing/amd64/
deb http://dist.yandex.ru/storage-common testing/all/
deb http://dist.yandex.ru/storage-common testing/amd64/
EOF

cat > /etc/apt/sources.list.d/yandex-search-trusty.list << EOF
deb http://search-trusty.dist.yandex.ru/search-trusty stable/all/
deb http://search-trusty.dist.yandex.ru/search-trusty stable/amd64/
EOF

cat > /etc/apt/sources.list.d/yandex-search.list << EOF
deb http://search.dist.yandex.ru/search stable/all/
deb http://search.dist.yandex.ru/search stable/amd64/
deb http://search.dist.yandex.ru/search unstable/all/
deb http://search.dist.yandex.ru/search unstable/amd64/
EOF

echo "deb http://mirror.yandex.ru/ubuntu trusty main restricted universe multiverse" > /etc/apt/sources.list

# Do not mount anything at boot
sed -e 's/^/#/g' -i lib/init/fstab

debconf-set-selections -v <<EOF
debconf debconf/frontend        select  Noninteractive
debconf debconf/priority        select  low
dash    dash/sh boolean false
openssh-server  openssh-server/permit-root-login        boolean false
postfix postfix/mailbox_limit   string  0
postfix postfix/relay_restrictions_warning      boolean
postfix postfix/tlsmgr_upgrade_warning  boolean
postfix postfix/main_mailer_type        select  No configuration
postfix postfix/procmail        boolean
postfix postfix/relayhost       string
postfix postfix/mydomain_warning        boolean
postfix postfix/mailname        string  /etc/mailname
postfix postfix/sqlite_warning  boolean
postfix postfix/recipient_delim string  +
postfix postfix/retry_upgrade_warning   boolean
postfix postfix/root_address    string
postfix postfix/mynetworks      string  127.0.0.0/8 [::ffff:127.0.0.0]/104 [::1]/128
postfix postfix/kernel_version_warning  boolean
postfix postfix/chattr  boolean false
postfix postfix/protocols       select
postfix postfix/not_configured  error
postfix postfix/rfc1035_violation       boolean false
postfix postfix/bad_recipient_delimiter error
postfix postfix/destinations    string
tzdata  tzdata/Zones/Africa     select
tzdata  tzdata/Zones/Antarctica select
tzdata  tzdata/Zones/Europe     select  Moscow
tzdata  tzdata/Zones/Atlantic   select
tzdata  tzdata/Zones/Australia  select
tzdata  tzdata/Zones/Asia       select
tzdata  tzdata/Zones/Arctic     select
tzdata  tzdata/Areas    select  Europe
tzdata  tzdata/Zones/America    select
tzdata  tzdata/Zones/SystemV    select
tzdata  tzdata/Zones/US select
tzdata  tzdata/Zones/Etc        select  UTC
tzdata  tzdata/Zones/Pacific    select
tzdata  tzdata/Zones/Indian     select
EOF

update-locale --no-check --reset $LOCALE_VARS
echo "$TZ" > /etc/timezone
dpkg-reconfigure tzdata

apt-get purge -y --force-yes yandex-conf-repo-unstable || true
rm /etc/apt/sources.list.d/yandex-trusty-unstable.list || true
apt-get update -qq

time apt-get --yes --no-install-recommends --force-yes install yandex-archive-keyring
time apt-get --yes --no-install-recommends --force-yes upgrade

apt-get --yes --no-install-recommends install --force-yes aria2 atop bc binutils bsd-mailx bsdmainutils bzip2 coreutils curl daemon dnsutils dstat ethtool file gawk gdb htop iptables iputils-ping locales language-pack-en less lockfile-progs logrotate lsb-release lsof ltrace mc moreutils most multitail ncdu netbase netcat netcat6 netcat-openbsd net-tools ntpdate openssh-client openssh-server procinfo psmisc pv rsync screen strace sudo tcpdump telnet vim vmtouch wget xz-utils

dpkg-reconfigure dash
echo 'root: cocaine-admin@yandex-team.ru' >>/etc/aliases
newaliases

#cleanup
apt-get clean
find /var/cache/apt/archives -type f -name '*.deb' -exec rm -f {} +
rm -rf /var/lib/apt/lists/*
rm -f /var/cache/apt/*.bin
rm -rf /var/log/installer
find /var/log/ -iname '*.gz' -exec rm -f {} +
for i in `find /var/log/ -iname *.1`
do
    rm $i
done
find /var/log -type f -exec truncate -s0 {} +

mv /etc/issue /etc/porto_image.info
echo "Base layer created on $(date -u +%F@%T) UTC" >>/etc/porto_image.info
ln -sf porto_image.info /etc/issue


PROJECT=testing
mkdir /build

if [ "$PROJECT" == "testing" ]
then
    echo "testing" > /build/env
    echo "ape-test-cloud-12" > /build/c_group
    echo "cloud-12" > /build/main_sls
    echo "qloud-cloud" > /build/qloud_sls
fi
if [ "$PROJECT" == "stable" ]
then
    echo "stable" > /build/env
    echo "ape-cloud-12-stable" > /build/c_group
    echo "cloud-12" > /build/main_sls
    echo "qloud-cloud" > /build/qloud_sls
fi

echo `curl -s -k https://c.yandex-team.ru/api/groups2hosts/\`cat /build/c_group\` | head -n1` > /build/hostname
export HOSTNAME=`cat /build/hostname`

apt-get update -qq
echo '......Install runit!.....'
apt-get install -yf runit || true

apt-get install --force-yes -y cgroup-lite || true
sed -i 's/invoke-rc.d cgroup-lite start || exit $?/invoke-rc.d cgroup-lite start || true/' /var/lib/dpkg/info/cgroup-lite.postinst
sed -i 's@update-rc.d -f cgroup-lite remove >/dev/null || exit $?@update-rc.d -f cgroup-lite remove >/dev/null || true@' /var/lib/dpkg/info/cgroup-lite.postinst
dpkg --configure cgroup-lite || true

apt-get --force-yes --yes install git libfastjson libjemalloc1 yabs-graphite-sender yabs-graphite-sender-config-corba python-requests yandex-internal-root-ca openssh-server=1:6.6p1-0yandexisolationed0 openssh-client=1:6.6p1-0yandexisolationed0 openssh-sftp-server=1:6.6p1-0yandexisolationed0 nginx python-support yandex-gosky

export P2P_WAIT_TIMEOUT=1
echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
apt-get --force-yes --yes install `curl -s -k https://c.yandex-team.ru/api/packages_on_host/\`cat /build/hostname\` |awk '{printf("%s=%s\n",$1,$3)}'|grep -E "cocaine|salt|yandex-3132-fastcgi-loggiver|config-media-admins-public-keys|yandex-push-client|p2p|yandex-environment|uatraits|lang-detect|regional|geobase5|libmds0libfolly0|libmetrics" | grep -vE "yandex-media-common-salt-minion-meta|ssh|omnibox|-dbg|libcocaine-" | xargs`

apt-get download yandex-porto config-monitoring-common iptruler config-media-graphite yandex-monitoring-scripts yandex-timetail config-monitoring-ubic-flaps monrun
dpkg --unpack *.deb
rm /var/lib/dpkg/info/yandex-porto.postinst -f
dpkg --configure yandex-porto || exit 0

#change hostname to install monrun and return in after install
cp -v /usr/bin/monrun /usr/bin/monrun.backup
sed -i "s/gethostname.fqdn(errback=addFQDNMessage)/'`cat /build/hostname`'/g" /usr/bin/monrun
sed -i "s/default=getHostName()/default='`cat /build/hostname`'/g" /usr/bin/monrun
apt-get install -yf || true
mv -v /usr/bin/monrun.backup /usr/bin/monrun

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

echo "Application binary layer created on $(date -u +%F@%T) UTC" >>/etc/porto_image.info



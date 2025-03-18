set -ex
rm /etc/apt/sources.list.d/*

echo 'deb http://search.dist.yandex.ru/search unstable/amd64/' >> /etc/apt/sources.list.d/dist.list
echo 'deb http://search.dist.yandex.ru/search stable/all/' >> /etc/apt/sources.list.d/dist.list
echo 'deb http://dist.yandex.ru/yandex-xenial stable/all/' >> /etc/apt/sources.list.d/dist.list
echo 'deb http://dist.yandex.ru/yandex-xenial stable/amd64/' >> /etc/apt/sources.list.d/dist.list
echo 'deb http://dist.yandex.ru/common stable/all/' >> /etc/apt/sources.list.d/dist.list

apt-get update
apt-get install -y graphviz
apt-get install -y yandex-internal-root-ca
apt-get install -y heirloom-mailx

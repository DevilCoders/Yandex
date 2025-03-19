#!/bin/bash
set -ev

# Fix the issue that localhost doesn't resolve into ::1
sed -i "s/\(^.*ip6-loopback\).*/\1 localhost/" /etc/hosts

cat > /etc/apt/sources.list.d/dist-yandex.list <<EOF
deb http://yandex-cloud.dist.yandex.ru/yandex-cloud stable/all/
deb http://yandex-cloud.dist.yandex.ru/yandex-cloud stable/amd64/
deb http://yandex-cloud.dist.yandex.ru/yandex-cloud testing/all/
deb http://yandex-cloud.dist.yandex.ru/yandex-cloud testing/amd64/
deb http://yandex-cloud.dist.yandex.ru/yandex-cloud unstable/all/
deb http://yandex-cloud.dist.yandex.ru/yandex-cloud unstable/amd64/
deb http://dist.yandex.ru/common/ stable/all/
deb http://dist.yandex.ru/common/ stable/amd64/
EOF

#curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
curl http://dist.yandex.ru/REPO.asc | sudo apt-key add -

apt-get -y update
apt-get -y install apt-transport-https
#add-apt-repository \
#   "deb [arch=amd64] https://download.docker.com/linux/ubuntu \
#   $(lsb_release -cs) \
#   stable"

apt-get -y update 
apt-get -q -y install python
#apt-get -q -y install docker-ce

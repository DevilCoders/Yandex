#!/bin/bash

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-clean.sh

set -ev

source /opt/yc-marketplace/yc-clean.sh

systemd-run --property="After=apt-daily.service apt-daily-upgrade.service" --wait /bin/true
apt-get -y install apt-transport-https ca-certificates curl software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | apt-key add -
add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"
apt-get -y update
apt-get -y install docker-ce jq golang

service docker start;
docker pull direvius/yandex-tank;
service docker stop;

cat > /usr/bin/yandex-tank <<EOF
#!/bin/bash

if [[ "\$EUID" = 0 ]]; then
    docker run --entrypoint yandex-tank -v \$(pwd):/var/loadtest -v \$HOME/.ssh:\$HOME/.ssh --net host direvius/yandex-tank \$@
elif [[ \`groups | grep docker\` ]]; then
    docker run --entrypoint yandex-tank -v \$(pwd):/var/loadtest -v \$HOME/.ssh:\$HOME/.ssh --net host direvius/yandex-tank \$@
else
    echo "You need 'sudo' privileges for use this script"
    echo "Try to run with sudo"
    sudo -k
    if sudo true; then
        sudo docker run --entrypoint yandex-tank -v \$(pwd):/var/loadtest -v \$HOME/.ssh:\$HOME/.ssh --net host direvius/yandex-tank \$@
    else
        echo "Can not take 'sudo' privileges"
        exit 1
    fi
fi

EOF

chmod +x /usr/bin/yandex-tank

systemctl enable docker

generate_print_credentials;
YC_WELCOME_BODY=""
YC_WELCOME_HELP="Documentation for Yandex Tank images available at http://yandextank.readthedocs.io/en/latest/tutorial.html"
print_msg;
on_install;

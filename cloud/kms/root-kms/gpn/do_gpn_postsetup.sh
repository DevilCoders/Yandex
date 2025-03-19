#!/bin/bash

set -ex

# Use DNS proxy.
pssh run "sudo sed -i \"/^nameserver /d\" /etc/resolv.conf; echo -e \"nameserver 2a0d:d6c0:2:200::3ea\nnameserver 2a0d:d6c0:2:201::2f4\nnameserver 2a0d:d6c0:2:202::302\" | sudo tee -a /etc/resolv.conf" $KMS_HOST_NAME

# Remove ntp to enable systemd-timesyncd
pssh run "sudo apt-get purge ntp -y" $KMS_HOST_NAME
pssh run "sudo sed -i \"/^NTP=/d\" /etc/systemd/timesyncd.conf; echo \"NTP=ntp.proxy.gpn.yandexcloud.net\" | sudo tee -a /etc/systemd/timesyncd.conf; sudo systemctl restart systemd-timesyncd" $KMS_HOST_NAME

# Use proxy for dist.
pssh run "sudo find /etc/apt -type f -exec sed -i \"s/yandex-cloud.dist.yandex.ru/yandex-cloud.dist.proxy.gpn.yandexcloud.net/g\" \{\} \;" $KMS_HOST_NAME
# Disable all other sources.
pssh run "sudo find /etc/apt -type f -exec sed -i \"s/\(^[^#].*dist.yandex.ru.*$\)/#\1/g\" \{\} \;" $KMS_HOST_NAME
# Enable upstream sources
pssh run "echo -e \"deb http://yandex-cloud-upstream-xenial-secure.dist.yandex.ru/yandex-cloud-upstream-xenial-secure stable/all/\ndeb http://yandex-cloud-upstream-xenial-secure.dist.yandex.ru/yandex-cloud-upstream-xenial-secure stable/amd64/\" | sudo tee /etc/apt/sources.list.d/yandex-cloud-upstream.list" $KMS_HOST_NAME

# Install push client token file
ya vault get version $PUSH_CLIENT_SEC -o logbroker_oauth_token > push_client_token
pssh scp push_client_token $KMS_HOST_NAME:
pssh run "sudo mv push_client_token /etc/kms/push_client_token && sudo chmod 600 /etc/kms/push_client_token && sudo chown push-client-user:push-client-user /etc/kms/push_client_token" $KMS_HOST_NAME
rm push_client_token

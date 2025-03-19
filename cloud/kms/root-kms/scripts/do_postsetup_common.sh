#!/bin/bash

# This script should be run after setting up a new host:
#  * sets hostname, including FQDN in /etc/hosts
#  * fixes sshd config, sets up sending sshd logs to the trapdoor
#  * installs osquery

set -ex

# Set hostname
SHORTNAME=`echo $KMS_HOST_NAME | cut -d. -f1`
pssh run "echo $SHORTNAME | sudo tee /etc/hostname; sudo hostname $SHORTNAME" $KMS_HOST_NAME
pssh run "sudo sed -i \"s/^127.0.1.1.*$//g\" /etc/hosts; echo \"127.0.1.1 $KMS_HOST_NAME $SHORTNAME\" | sudo tee -a /etc/hosts" $KMS_HOST_NAME

# Fix sshd_config
pssh run "sudo sed -i \"s/^FingerprintHash.*$//g\" /etc/ssh/sshd_config; sudo sed -i \"s/^LogLevel.*$//g\" /etc/ssh/sshd_config; echo -e \"FingerprintHash sha256\nLogLevel VERBOSE\" | sudo tee -a /etc/ssh/sshd_config" $KMS_HOST_NAME
# Add trapdoor
pssh run "echo \"auth,authpriv.* @trapdoor.yandex.net\" | sudo tee /etc/rsyslog.d/99-ssh-trapdoor.conf; sudo systemctl restart ssh; sudo systemctl restart rsyslog" $KMS_HOST_NAME

# Install the certificate.pem
. `dirname $0`/install_certificate_common.sh

# fix ntp.config
pssh run "sudo sed -i \"/pool.*ubuntu.*/d\" /etc/ntp.conf; echo -e \"server ntp1.yandex.net iburst burst prefer\nserver ntp2.yandex.net iburst burst\nserver ntp3.yandex.net iburst burst\nserver ntp4.yandex.net iburst burst\n\" | sudo tee -a /etc/ntp.conf" $KMS_HOST_NAME
pssh run "sudo apt-get install ntpdate; sudo service ntp stop; sudo ntpdate ntp1.yandex.net; sudo service ntp start" $KMS_HOST_NAME

# Create push-client user and dirs
pssh run "sudo mkdir -p /var/lib/push-client" $KMS_HOST_NAME
pssh run "sudo mkdir -p /var/spool/push-client" $KMS_HOST_NAME
pssh run "sudo useradd -rmb /var/lib/push-client -s /sbin/nologin push-client-user || :" $KMS_HOST_NAME
pssh run "sudo chown push-client-user:push-client-user /var/spool/push-client" $KMS_HOST_NAME

# Install the tvm_secret
if [ ! -z "$TVM_SEC" ]
then
    ya vault get version $TVM_SEC -o client_secret > tvm_secret
    pssh scp tvm_secret $KMS_HOST_NAME:
    pssh run "sudo mv tvm_secret /etc/kms/tvm_secret && sudo chmod 600 /etc/kms/tvm_secret && sudo chown push-client-user:push-client-user /etc/kms/tvm_secret" $KMS_HOST_NAME
    rm tvm_secret
fi

`dirname $0`/install_osquery.sh

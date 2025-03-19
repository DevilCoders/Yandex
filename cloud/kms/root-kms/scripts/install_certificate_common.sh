#!/bin/bash

ya vault get version $CERTIFICATE_SEC -o certificate_pem > root-certificate.pem
pssh scp root-certificate.pem $KMS_HOST_NAME:
# for root-kms
pssh run "sudo mkdir -p /etc/kms" $KMS_HOST_NAME
pssh run "sudo mkdir -p /var/lib/kms" $KMS_HOST_NAME
pssh run "sudo useradd -rmb /var/lib/kms -s /sbin/nologin kms-root-service || :" $KMS_HOST_NAME
pssh run "sudo cp root-certificate.pem /etc/kms/certificate.pem && sudo chmod 600 /etc/kms/certificate.pem && sudo chown kms-root-service:kms-root-service /etc/kms/certificate.pem" $KMS_HOST_NAME
# for hsm-front
pssh run "sudo mkdir -p /etc/kms-hsm-front" $KMS_HOST_NAME
pssh run "sudo mkdir -p /var/lib/kms-hsm-front" $KMS_HOST_NAME
pssh run "sudo useradd -rmb /var/lib/kms-hsm-front -s /sbin/nologin kms-hsm-front-service || :" $KMS_HOST_NAME
pssh run "sudo mv root-certificate.pem /etc/kms-hsm-front/certificate.pem && sudo chmod 600 /etc/kms-hsm-front/certificate.pem && sudo chown kms-hsm-front-service:kms-hsm-front-service /etc/kms-hsm-front/certificate.pem" $KMS_HOST_NAME
rm root-certificate.pem

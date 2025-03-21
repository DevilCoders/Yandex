#!/bin/bash -eux

BRANCH=v1.6.3+yckms # the branch from https://github.com/yandex-cloud/vault.git will be used to build the image

# Install Go
wget https://golang.org/dl/go1.16.1.linux-amd64.tar.gz # https://golang.org/doc/install
rm -rf /usr/local/go && tar -C /usr/local -xzf go1.16.1.linux-amd64.tar.gz # https://golang.org/doc/install
export GOPATH=~/go
export PATH=$PATH:/usr/local/go/bin:$GOPATH/bin

# Install the required packages
apt update -y
curl -fsSL https://deb.nodesource.com/setup_10.x | sudo -E bash - # https://github.com/nodesource/distributions#installation-instructions
apt install -y git build-essential nodejs
npm install -g yarn

# Checkout source code from branch $BRANCH and build Vault binary
git clone https://github.com/yandex-cloud/vault.git
cd vault
git checkout $BRANCH
make bootstrap static-dist dev-ui

# Uninstall the packages
npm uninstall -g yarn
apt purge -y git build-essential nodejs

# Uninstall Go
rm -rf /usr/local/go

# Move Vault binary to /usr/local/bin
mv bin/vault /usr/local/bin/
chown root:root /usr/local/bin/vault
setcap cap_ipc_lock=+ep /usr/local/bin/vault

# Prepare Vault config
useradd --system --home /etc/vault.d --shell /bin/false vault
mkdir --parents /etc/vault.d
cat > /etc/vault.d/vault.hcl << EOF
# See https://www.vaultproject.io/docs/configuration for more details about configuration options

ui = true

storage "file" {
  path = "/opt/vault"
}

# HTTP listener (insecure)
listener "tcp" {
  address = "127.0.0.1:8200"
  tls_disable = 1
}

## HTTPS listener
#listener "tcp" {
#  address       = "0.0.0.0:8200"
#  tls_cert_file = "tls.crt"
#  tls_key_file  = "tls.key"
#}

## Auto Unseal via Yandex Key Management Service (see https://cloud.yandex.ru/docs/kms/solutions/vault-secret for more details)
#seal "yandexcloudkms" {
#  kms_key_id = "YOUR-YANDEX-CLOUD-KMS-KEY-ID"
#}
EOF
chown --recursive vault:vault /etc/vault.d
chmod 640 /etc/vault.d/vault.hcl
mkdir /opt/vault
chown --recursive vault:vault /opt/vault

# Prepare Vault systemd unit file
cat > /etc/systemd/system/vault.service << EOF
[Unit]
Description="HashiCorp Vault - A tool for managing secrets"
Documentation=https://www.vaultproject.io/docs/
Requires=network-online.target
After=network-online.target
ConditionFileNotEmpty=/etc/vault.d/vault.hcl
StartLimitIntervalSec=60
StartLimitBurst=3

[Service]
User=vault
Group=vault
ProtectSystem=full
ProtectHome=read-only
PrivateTmp=yes
PrivateDevices=yes
SecureBits=keep-caps
AmbientCapabilities=CAP_IPC_LOCK
CapabilityBoundingSet=CAP_SYSLOG CAP_IPC_LOCK
NoNewPrivileges=yes
ExecStart=/usr/local/bin/vault server -config=/etc/vault.d/vault.hcl
ExecReload=/bin/kill --signal HUP $MAINPID
KillMode=process
KillSignal=SIGINT
Restart=on-failure
RestartSec=5
TimeoutStopSec=30
StartLimitInterval=60
StartLimitBurst=3
LimitNOFILE=65536
LimitMEMLOCK=infinity

[Install]
WantedBy=multi-user.target
EOF

# Enable Vault unit
systemctl enable vault

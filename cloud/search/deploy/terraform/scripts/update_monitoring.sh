#!/bin/bash

set -e
set -o pipefail

LOG_FILE="/var/log/update_monitoring.log"

to_log() {
    echo "$(date +''"''"''%Y-%m-%d %H:%M:%S''"''"'') [$$] $1" | tee -a "$LOG_FILE"
}

to_log "Start script"

to_log "add repo"
echo -e ''"''"''deb http://yandex-cloud.dist.yandex.ru/yandex-cloud stable/all/\ndeb http://yandex-cloud.dist.yandex.ru/yandex-cloud stable/$(ARCH)/''"''"'' > /etc/apt/sources.list.d/yandex-cloud.list

to_log "apt update"
apt update ||:
to_log "install deb pkgs"
apt install yandex-solomon-agent-bin=${packet_version} --yes --allow-downgrades

#### PREPARE CONFIGS ####
LOG_DIR="/var/log/yc/yandex-solomon-agent/"
to_log "create and chown $LOG_DIR"
mkdir -p $LOG_DIR
chown nobody:nogroup $LOG_DIR

CONFIG_DIR="/etc/yandex-solomon-agent/services"
to_log "create $CONFIG_DIR"
mkdir -p $CONFIG_DIR

to_log "create agent.conf"
echo -e ''"''"''${agent_conf}''"''"'' > /etc/yandex-solomon-agent/agent.conf

to_log "create system.conf"
echo -e ''"''"''${system_conf}''"''"'' > $CONFIG_DIR/system.conf

to_log "create yc-search-stat.conf"
echo -e ''"''"''${yc_search_stat_conf}''"''"'' > $CONFIG_DIR/yc-search-stat.conf
#########################

#### PREPARE SYSTEMD ####
SERVICE_NAME="solomon-agent"
SYSTEMD_CFG="/usr/lib/systemd/system/$SERVICE_NAME.service"
to_log "create $SYSTEMD_CFG"
echo -e ''"''"''${systemd_cfg}''"''"'' > $SYSTEMD_CFG

to_log "Start $SERVICE_NAME"
systemctl daemon-reload
systemctl enable --no-block $SERVICE_NAME.service
systemctl start --no-block $SERVICE_NAME.service
#########################

to_log "Finish script"

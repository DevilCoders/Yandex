#!/bin/bash

set -e
set -o pipefail

LOG_FILE="/var/log/per_boot_yc.log"

to_log() {
    echo "$(date +''"''"''%Y-%m-%d %H:%M:%S''"''"'') [$$] $1" | tee -a "$LOG_FILE"
}

to_log "Start script"

to_log "apt update"
apt update ||:
to_log "install deb pkgs"
apt install ${packet_name}=${packet_version} yandex-internal-root-ca --yes --allow-downgrades

#### PREPARE SEARCHMAP ####
to_log "Start prepare searchmap"
rm -f /usr/yc/search/searchmap_yc.txt
echo -e ''"''"''${searchmap}''"''"'' > /usr/yc/search/searchmap_yc.txt
chown yc_search: /usr/yc/search/searchmap_yc.txt
to_log "Finish prepare searchmap"
###########################

#### PREPARE KEYS ####
to_log "Install manual keys"
FILE="/home/ubuntu/.ssh/authorized_keys";
to_log "Install okkk keys"
LINE="ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAACAQDKrp448ZYcjO9OP3FnKYUHuA6iNbrH/M6DOHT7FIynH0XdO0FMglZIvzOkgzjwdzL9W9JzA7de8rsI1XYBpmhNy5dFNLJlZGvnI+RI3amnCtryl8UmYhdGQCbFMXkOAl7S6hm3uVLIFm4V1W6g2ECScC9plChIbbgGDgRAYNLgVnkw7YvjsBiCeApApYf+/I8EuGQDwAV/RouKBZ3cGA5Y/hdJnJgWJIFmQx1cfVX2s3PD8xLhTXsS2rWLbwVUj+w8WDQUueUfD+l2w1F94o4BeDbjZ2VQWwcnRcyg0TOZt/OsW6t9LfUPTd3jl1DtwrNR9w+XzmcSf3G7X3aWq7+XHgOa1FIYLeyKmfKWdZRxMYkA0xcObSqt7uwjHoUHRgH8HtlkTfmIJINh5EKPuqMSJiORKUVuAei18jsjNMNw20qMbmpRAx4h0KpZzNknej86kn57u3kScxmMXuOaS70hHiXzca50JtrbsvlQhCosKyPJorlUeWf33uy823M94iT4Rutufm9ujl0CLnXa0n1v5alq6R2qhu6TMXBLoR1+8387/TY7uPXOjSC6us3Rv0iT6hThzbL/KBg7riHAx+zrpocRVHoizfpz9qf4FPwe1r46y3nSP3baLQOHLe/OFyStGQyZGHLZQ6howGfgzZViES9jHSs4NUpafenatXkYSQ== okkk@Sprite"
grep -qF -- "$LINE" "$FILE" || echo "$LINE" >> "$FILE"
to_log "Install ggalina13 keys"
LINE="ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAACAQCjzjaTVqUheJMUoz5S1COfz4/WZdkatihvEmc9V3P39xwjdAgmE8fn8qbPJ79B2q5BfS/CzMLjmtI+cTq3CmLmdPecRVj7goDb1+v1QD3oX64zpdl9uSEjEfblLUY5uOsaorMKtPxTgqj3/MEnLhOqv7U9SI1nKqIIfItM7UjxC4TtvRN8OHVqvtjs1wDu95Z6Iniq/eaDanmeEvhX7q9kOjuv9zQJ8n6Kq0kNG+7cpbFXAFtVDSKNTNU73PHlW265SMjeXkNu46K8hbv1o2WQXDOG2XQGX6MkcRgd5fIdKtxUcNA+T6xMSZtVNJPmoL/iCLrC4OkKvuWgiBV87N/cEPQga/vWDMBiuHx3B+z7PrRtP6d+lVdW5bB2dqHXBfqoKNBQ7ApPHZ7/aWFULSCARduYTO/8KyRuJIbcqRxK5S8Xjsj4I0YYicBVNE4Gr/BqlVUu1SoBeguBZ5OXNDX3u8KQ4Mzrdy/rTljqLfka1tzXl2lF5+lG9b33YZcME/djbhQA2Ce2QPhOnBk7fwEwu/EKrfPj0UiAM0Pfwf9SfGNLt9+IHBAMHOGUpWE1ONJAzFtgtF08bh81+t7UrwPhl8E/rI97XJQr7cf1SWzWd656JqBpOStdsIC9VYRS4i8v+XJ1nyowepuXrjc0ne5Jk4gJZcMWkHXz2XFWhHKvGQ== ggalina13@i104293064"
grep -qF -- "$LINE" "$FILE" || echo "$LINE" >> "$FILE"
to_log "Finish install manual keys"
###########################

#### GENERATE SECRETS ####
to_log "Start generate secrets"
selfdns_secret="${selfdns_secret}"
sa_consumer_secret="${sa_consumer_secret}"
sa_consumer_version="${sa_consumer_version}"

to_log "Get IAM token from computeMetadata"
export IAM_TOKEN=$(curl -s -H Metadata-Flavor:Google http://169.254.169.254/computeMetadata/v1/instance/service-accounts/default/token | jq -r .access_token)

to_log "Get selfdns token from lockbox"
export SELFDNS_TOKEN=$(curl -s -X GET -H "Authorization: Bearer $IAM_TOKEN" "https://${lockbox_api_host}/lockbox/v1/secrets/$selfdns_secret/payload" | jq -r .entries[].textValue)
sed -i "/token/s/.*/token = $SELFDNS_TOKEN/" /etc/yandex/selfdns-client/default.conf

to_log "Get consumer service account creds from lockbox"
sa_file="/etc/sa_consumer.json"
curl -s -X GET -H "Authorization: Bearer $IAM_TOKEN" "https://${lockbox_api_host}/lockbox/v1/secrets/$sa_consumer_secret/payload?versionId=$sa_consumer_version" | jq -r ''"''"''.entries[] | select(.key == "sa_consumer.json") | .binaryValue''"''"'' | base64 -d > $sa_file
chown yc_search: $sa_file
chmod 0600 $sa_file
to_log "Finish generate secrets"
##########################

#### PREPARE ZK CONFIG ####
ZKCFG=/usr/yc/search/yc_search_queue/yc_search_queue.conf
if [ -f "$ZKCFG" ]; then
    to_log "Start prepare zk config"
    %{ for fqdn, number in zk_config }
    sed -i "/server.${number}/s/.*/server.${number}=${fqdn}:8082:8083/" $ZKCFG
    %{ endfor ~}

    export ZKID=$(grep $(hostname -f) /usr/yc/search/yc_search_queue/yc_search_queue.conf | cut -d "=" -f1 | cut -d "." -f2)
    sed -i "s/$(hostname -f)/0.0.0.0/" $ZKCFG
    echo $ZKID > /media/yc/search/queue/data/myid
    to_log "Finish prepare zk config"
fi
###########################

#### PREPARE SERVICE CONFIG ####
SERVICE_CFG_PATH="/usr/yc/search/${service_name}/${service_name}_config.sh"
SERVICE_CFG_ENV_PATH="/usr/yc/search/${service_name}/${env_name}/${service_name}_config.sh"
if [ -f "$SERVICE_CFG_ENV_PATH" ]; then
  to_log "Create link $SERVICE_CFG_PATH -> $SERVICE_CFG_PATH"
  rm -f "$SERVICE_CFG_PATH"
  ln -s "$SERVICE_CFG_ENV_PATH" "$SERVICE_CFG_PATH"
  to_log "Link has been created"
fi
################################

to_log "Start systemd service"
systemctl daemon-reload
systemctl enable --no-block ${service_name}.service
systemctl start --no-block ${service_name}.service

to_log "Finish script"

#!/bin/bash

# Export bootstrap variables
export $(cat /proc/1/environ | tr '\0' ' ')
# pkgs to install

PKGS="
dpkg
config-monitoring-pkgver
config-media-admins-public-keys
yandex-media-common-salt-auto
"

CLEANUP_PKGS="
console-setup
keyboard-configuration
open-iscsi
irqbalance
open-vm-tools
pppconfig
ppp
landscape-client
"

DISABLE_INIT_SCRIPTS="
mdadm
mcelog
smartmontools
ipmievd
wd_keepalive
ondemand
watchdog
openipmi
mountdebugfs
sysfsutils
sysstat
"

OS_NAME=$(lsb_release -a 2>1 | grep Codename | awk '{ print $2 }')

function log() {
    # Log into stderr
    local ts=$(date +%Y-%m-%dT%H:%M:%S)
    echo -e "$ts\t[bootstrap]\t$1" >&2
}

function is_projectid() {
    prjid="/etc/network/projectid"
    if [ -f "$prjid" ] && [ ! -z $(cat $prjid) ]; then
        return 0
    fi
    return 1
}

function manage_resolvconf() {
    # No need in resolvconf
    log "Checking /etc/resolv.conf, remove symlink if present"
    cp --remove-destination $(readlink /etc/resolv.conf) /etc/resolv.conf |& :
}

function manage_hostname() {
    local domain="${1##.}" # remove leading dot
    local sed_re='s/--/./g;s/\.'"$domain"'$//;s/(.*)/\1.'"$domain/"

    if [ -z "$domain" ]; then
        log "host domain not set, do not modify container hostname"
        return
    fi

    log "checking container hostname, domain: $domain"
    hostname $(hostname | sed -r "$sed_re")
    hostname >/etc/hostname
    log "hostname set to $(hostname)"
}

function manage_dns() {
    local token="$1"
    if [ -z "$token" ]; then
        log "token variable null, skip selfdns syncing"
        return
    fi

    log "running selfdns with provided token"
    /usr/bin/selfdns-client --token "$token" --terminal
}

function _http() {
    local mode="$1"
    local url="$2"
    local token="$3"
    local data="$4"
    local req="-H 'Accept: application/json' -H 'Content-Type: application/json'"

    if [ ! -z "$token" ]; then
        req="$req -H 'Authorization: OAuth $token'"
    fi

    if [[ "$mode" == "code" ]]; then
        req="$req -o /dev/null -w '%{http_code}'"
    elif [[ "$mode" == "body" ]]; then
        req="$req -k"
    else
        log "Unknown curl mode! Seems bug, exiting!"
        exit 1
    fi

    if [ ! -z "$data" ]; then
        req="$req -d '$data'"
    fi

    eval "curl -s $req $url"
}

function manage_conductor() {
    local group="$1"
    local domain="$2"
    local token="$3"
    local no_ra_settings="$4"
    if [ -z "$token" ] || [ -z "$group" ]; then
        log "conductor token or group not set, skip conductor for this host"
        return
    fi

    log "trying to add host to conductor group: $group"
    local status=$(
        _http "code" "https://c.yandex-team.ru/api/v1/hosts" "$token" \
            '{"host": {
            "fqdn": "'$(hostname)'",
            "short_name": "'$(hostname | sed s/"$domain"//g)'",
            "group": {"name": "'"$group"'"}
        }}'
    )
    if [[ "$status" == "201" ]]; then
        log "Added host to conductor group: $group"
    elif [[ "$status" == "422" ]]; then
        log "Host already in conductor"
    else
        log "Unexpected answer from conductor: $status"
    fi

    if [ -z $no_ra_settings ] && ! is_projectid; then
        log "trying to add RA tag"
        local tag_id=$(
            _http "body" "https://c.yandex-team.ru/api/v1/tags/RA" "$token" |
                egrep -o '"id":[0-9]+'
        )
        if [ -z "$tag_id" ]; then
            log "RA tag not found in conductor, you need to create it"
            return
        fi

        log "Found tag, adding to host"
        status=$(
            _http "code" "https://c.yandex-team.ru/api/v1/hosts/$(hostname)/tags" \
                "$token" '{"tag": {'$tag_id'}}'
        )
        if [[ "$status" == "201" ]]; then
            log "Added RA tag to host $(hostname)"
        elif [[ "$status" == "422" ]]; then
            log "Ra tag already present"
        else
            log "Unexpected status from conductor: $status"
        fi
    fi
}

function manage_package() {
    local pkg_name="$1"
    if dpkg -l "$pkg_name" &>/dev/null; then
        log "$pkg_name installed, skipping"
        return
    fi

    log "$pkg_name not installed, installing"
    apt-get -qq install "$pkg_name" -y --force-yes 1>/dev/null
}

function _manage_l2_network() {
    log "Installing config-interfaces, regenerating network"
    manage_package "config-interfaces"
    make -C /etc/network && ifdown -a
    ifup -a
}

function _manage_prjid_network() {
    log "Project id network, installing mock for config-interfaces"
    manage_package "yandex-media-common-configinterfacesmock"
    # regenerate network due to hostname change
    ifdown -a
    ifup -a
}

function manage_network() {
    if is_projectid; then
        _manage_prjid_network
    else
        _manage_l2_network
    fi
}

function manage_hbf() {
    local hbf="$1"
    local upstart_disable_file="/etc/yandex/disable-hbf-upstart_via-override"
    mkdir -p /etc/yandex && echo "manual" >$upstart_disable_file
    if [ -z "$hbf" ] || [ $hbf -eq 0 ]; then
        if [ ! -f "/etc/init/yandex-hbf-agent.override" ]; then
            ln -sf $upstart_disable_file /etc/init/yandex-hbf-agent.override
            service yandex-hbf-agent stop || true
        fi
        if [ -x /bin/systemctl ]; then
            /bin/systemctl disable yandex-hbf-agent.service || true
            /bin/systemctl mask yandex-hbf-agent.service || true
            /bin/systemctl stop yandex-hbf-agent.service
        fi
        /usr/bin/yandex-hbf-agent --disable-hbf
    else
        if [ -L "/etc/init/yandex-hbf-agent.override" ] &&
            [[ $(readlink /etc/init/yandex-hbf-agent.override) == "$upstart_disable_file" ]]; then
            rm -f /etc/init/yandex-hbf-agent.override
            service yandex-hbf-agent start || true
        fi
        if [ -x /bin/systemctl ]; then
            /bin/systemctl unmask yandex-hbf-agent.service || true
            /bin/systemctl enable /usr/share/yandex-hbf-agent-init/systemd/yandex-hbf-agent.service ||
                true
            /bin/systemctl start yandex-hbf-agent.service
        fi
    fi
}

function manage_grub() {
    log "Cleaning up grub.cfg"
    if test -f /boot/grub/grub.cfg && ! dpkg -l grub2-common 2>/dev/null; then
        rm -v /boot/grub/grub.cfg
    fi
}

function manage_cron() {
    log "Restart cron daemon"
    service cron restart || true
}

function cleanup_container() {
    for pkg in $CLEANUP_PKGS; do
        log "Purge package $pkg"
        apt-get -y purge $pkg
    done
    if [ $OS_NAME == 'trusty' ]; then
        for script in $DISABLE_INIT_SCRIPTS; do
            log "Disable init script $script"
            update-rc.d $script disable
            /etc/init.d/$script stop
        done
    fi
}

function disable_unattended_upgrade() {
    log "Disable unattended-upgrade binary"
    ln -vsfT /bin/true /usr/bin/unattended-upgrade
    ln -vsfT /bin/true /usr/bin/unattended-upgrades
    log "Disable unattended-upgrade config"
    sed -ri 's|^([^/])|//\1|' /etc/apt/apt.conf.d/50unattended-upgrades || true
}

function override_init_configs() {
    local tty_configs=$(ls -1 /etc/init/tty*.conf 2>/dev/null)
    local current_overrides=$(ls -1 /etc/init/tty*.override 2>/dev/null)
    for file in $tty_configs; do
        echo manual >${file%%.conf}.override
    done
    local updated_overrides=$(ls -1 /etc/init/tty*.override 2>/dev/null)
    local diff="$(
        diff -U0 <(echo "$current_overrides") <(echo "$updated_overrides")
    )"
    log "Overrides diff $diff"
}

log "Running $0"
manage_resolvconf
# projectid network may generate ip based on hostname, so firstly we need to regenerate hostname
manage_hostname "$LXD_CONTAINER_DOMAIN"

manage_conductor "$LXD_CONDUCTOR_GROUP" "$LXD_CONTAINER_DOMAIN" "$LXD_CONDUCTOR_TOKEN" "$LXD_MTN_RA_DISABLE"
if test -z "$LXD_SKIP_MANAGE_NETWORK"; then
    manage_network
fi
manage_hbf "$LXD_HBF"

if test -n "$LXD_DNS_TOKEN"; then
    manage_dns "$LXD_DNS_TOKEN"
fi

for pkg in $PKGS; do
    manage_package "$pkg"
done

if [ ! -z "$LXD_YANDEX_ENVIRONMENT" ]; then
    manage_package "yandex-environment-$LXD_YANDEX_ENVIRONMENT"
fi

manage_grub
manage_cron
cleanup_container
disable_unattended_upgrade
override_init_configs

log "Finished"

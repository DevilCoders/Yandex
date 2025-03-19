#!/bin/bash

set -ue -o pipefail

if [[ "${1:-}" =~ ^(-h|--help) ]]; then
    echo "$0 [y]
    y  Answer yes to all questions
    "
    exit 0
fi
DEFAULT_REPLY="${1:-}"
DEFAULT_REPLY="${DEFAULT_REPLY//-/}"

if test -d /snap/bin; then
    export PATH="$PATH:/snap/bin"
fi

port_net_prefix=$(ip -o a s dev vlan688 scope global | grep -Po 'inet6 \K.*(?=::badc:ab1e)')
vms="${LXD_CONDUCT_VMS:-$(lxc ls --format=csv -cn)}"

get_project_id() {
    local vm_name=$1
    local host_profile="$(lxc config show $vm_name | grep -A999 profiles | grep -Po '^-\s+\K(music|media).*')"
    local project_id=639
    for p in $host_profile; do
        project_id=$(grep -Po "user.prjid:\s+['\"]0x\K[^'\"]+" /etc/yandex/lxd/profiles/$p)
    done
    echo $project_id
}

get_host_name() {
    echo -n $1 | sed -r 's/--/./g;s/(.*)/\1.yandex.net/' | tr -d '\n'
}

get_host_ip() {
    local vm_name="$1"
    local net_prefix="$2"
    # host addr is 4 first symbols of md5sum of container hostname
    local host_addr=$(get_host_name "$vm_name" | md5sum | sed -r 's/^(....)(....)(.*)/\1:\2/')
    project_id=$(get_project_id $vm_name)
    echo "$net_prefix::$project_id:$host_addr"
}

if test -z "$DEFAULT_REPLY"; then
    read -p "reload profiles? (NO/yes): "
else
    REPLY=$DEFAULT_REPLY
fi
if test -z "LXD_CONDUCT_VMS" && [[ "${REPLY^^}" =~ ^Y ]]; then
    echo "Reload profiles"
    find /etc/yandex/lxd/profiles -type f |
        while read line; do
            name=$(echo $line | awk -F\/ '{print $NF}')
            lxc_name=$(lxc profile list | awk -v name=$name '$2==name {print $2}')
            if [[ "$name" != "$lxc_name" ]]; then
                lxc profile create $name
            fi
            cat $line | lxc profile edit $name
        done
fi

for vm in $vms; do
    fqdn=$(get_host_name $vm)
    ip=$(get_host_ip $vm $port_net_prefix)

    if test -z "$DEFAULT_REPLY"; then
        read -p "conduct $vm=$fqdn:$ip? (NO/yes): "
    else
        REPLY=$DEFAULT_REPLY
    fi
    if [[ "${REPLY^^}" =~ ^Y ]]; then

        echo "$vm $fqdn=$ip"
        echo "Cleanup profile $vm"
        rm -vf /etc/yandex/lxd/profiles/$vm
        if lxc profile show $vm &>/dev/null; then
            lxc profile remove $vm $vm
        fi

        echo "Fix network/interfaces"
        RC_LOCAL="$(lxc file pull $vm/etc/rc.conf.local >(cat) 2>/dev/null ||
            { lxc file pull $vm/etc/rc.conf.local /dev/shm/$vm.rc.conf.local 2>/dev/null; cat /dev/shm/$vm.rc.conf.local; } ||
            true)"
        if echo "$RC_LOCAL" | grep -q ya_slb; then
            echo "  Add ya-slb-tun start"
            up_slb="post-up /usr/lib/yandex-netconfig/ya-slb-tun start"
        fi

        INTERFACES="$(lxc file pull $vm/etc/network/interfaces >(cat) 2>/dev/null ||
            { lxc file pull $vm/etc/network/interfaces /dev/shm/$vm.interfaces 2>/dev/null; cat /dev/shm/$vm.interfaces; } ||
            true)"
        backup_interfaces_file="/root/interfaces-backup-$(date +%TT%F)"
        if test -n "$INTERFACES"; then
            echo "Backup origin /etc/network/interfaces to $backup_interfaces_file"
            echo "$INTERFACES" > /dev/shm/$vm
            lxc file push <(echo "$INTERFACES") $vm$backup_interfaces_file ||
                lxc file push /dev/shm/$vm $vm$backup_interfaces_file
        fi
        echo ---

        interfaces_config="auto lo
iface lo inet loopback
  ya-netconfig-disable yes

auto eth0
iface eth0 inet6 static
  ya-netconfig-disable yes
  address $ip/128
  gateway fe80::1
  ${up_slb:-}
"
        echo "Push /etc/network/interfaces to $vm"
        echo "$interfaces_config" > /dev/shm/$vm
        set -x
        lxc file push <(echo "$interfaces_config") $vm/etc/network/interfaces ||
            lxc file push /dev/shm/$vm $vm/etc/network/interfaces
        set +x
        echo =====================================

        echo "Fix /etc/hosts"
        HOSTS_FILE="$(lxc file pull $vm/etc/hosts >(cat) 2>/dev/null ||
            { lxc file pull $vm/etc/hosts /dev/shm/$vm.hosts 2>/dev/null; cat /dev/shm/$vm.hosts; } ||
            true)"
        backup_hosts_file="/root/hosts-backup-$(date +%TT%F)"
        if test -n "$HOSTS_FILE"; then
            echo "$HOSTS_FILE" > /dev/shm/$vm
            echo "Backup origin /etc/hosts to $backup_hosts_file"
            lxc file push <(echo "$HOSTS_FILE") $vm$backup_hosts_file ||
                lxc file push /dev/shm/$vm $vm$backup_hosts_file
        fi
        echo ---
        HOSTS_FILE="$(sed '/^2a02/d;/yandex.net/d' <<<"$HOSTS_FILE")"
        HOSTS_FILE="$HOSTS_FILE
$ip $fqdn ${fqdn%%.*}"

        echo "Push /etc/hosts to $vm:"
        echo "$HOSTS_FILE" > /dev/shm/$vm
        set -x
        lxc file push <(echo "$HOSTS_FILE") $vm/etc/hosts ||
            lxc file push /dev/shm/$vm $vm/etc/hosts
        set +x
        echo =====================================

        echo "Set lxc config for $vm"
        set -x
        lxc config set $vm user.ip "$ip"
        lxc config set $vm user.up_slb "${up_slb:-}"
        set +x
        echo =====================================

        if lxc ls $vm | grep RUNNING; then
            echo "Restart vm? $vm"
            if test -z "$DEFAULT_REPLY"; then
                read -p "Stop $vm? (NO/yes): "
            else
                REPLY=$DEFAULT_REPLY
            fi
            if ! [[ "${REPLY^^}" =~ ^Y(ES)?$ ]]; then
                echo "Skip restarting $vm"
                continue
            fi
            lxc restart $vm
        else
            retry=0
            until lxc start $vm; do
                if ((retry > 2)); then
                    echo "\[[1;31mFailed to start $vm\[[0m, skip"
                    continue
                fi
                let retry++
                echo "Retry start $vm #$retry"
            done
        fi
    fi
done

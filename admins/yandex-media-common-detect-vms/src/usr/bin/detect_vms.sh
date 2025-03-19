#!/usr/bin/env bash
#
# CADMIN-8562
#
# Detects if this dom0 host runs a VM from `excluded_vms.conf`
# If exit code is 0; then this host does run one of these VMs
# If exit code is 1; then this host does not run VMs from `excluded_vms.conf`
#
# Argument $1: check name. We exclude monitoring for concrete checks.
#

check_name=$1
. /etc/yandex/excluded_vms.conf

# Should be like in /etc/yandex/excluded_vms.conf (e.g. `cpu_check`, `network_load`)
#
exclude="exclude_$check_name"
exclude=${!exclude}

if [[ $exclude == "" ]]; then
    echo "Bad check name given for script /usr/bin/detect_vms.sh"
    exit 1
fi

vms_on_board=$(ps xo cmd | awk '/\[lxc monitor\]/ {print $NF}')

for vm in $vms_on_board; do
    vm_pattern=$(echo $vm | sed -e 's/[0-9]\{2\}[a-z]/.../')
    [[ $exclude =~ $vm_pattern ]] && exit 0 || continue
done

exit 1


# EOF


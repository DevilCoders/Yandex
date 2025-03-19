#!/bin/bash

function set_taps_link() {
    # CLOUD-22245 workaround for: vrouter.ko may crash at vr_fragment_enqueue() if fragmented packet received during module unloading.
    # Can be removed when root cause is fixed (CLOUD-16387)
    echo "Setting all tap/qvb interfaces link to $1..."
    for iface in $(cat /var/lib/contrail/ports/* | jq '."system-name"' | tr -d '"'); do ip link set dev $iface $1; done
}

if [[ -z "$1" ]]; then
    echo "USAGE: $(basename $0) VHOST_IFACE_NAME"
    exit 2
fi
vhost_iface_name="$1"

echo "$(date) contrail-vrouter-agent and vrouter.ko are going to RESTART at $(hostname)..."
set -x

systemctl stop contrail-vrouter-agent
set_taps_link down
ifdown $vhost_iface_name
rmmod vrouter

if ! modprobe vrouter; then
    echo "WARNING: 'modprobe vrouter' failed once, trying to compact memory (CLOUD-21413)."
    echo 1 > /proc/sys/vm/compact_memory
    if ! modprobe vrouter; then
        echo "ERROR: 'modprobe vrouter' failed twice, giving up. Overlay network on this host is BROKEN."
        set_taps_link up
        exit 1
    fi
fi

ifup $vhost_iface_name 1>>/tmp/ifup-vhost.stdout 2>>/tmp/ifup-vhost.stderr
set_taps_link up
systemctl start contrail-vrouter-agent

set +x
echo "$(date) completed."

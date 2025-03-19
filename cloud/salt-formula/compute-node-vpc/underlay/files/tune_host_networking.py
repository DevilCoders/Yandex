#!/usr/bin/env python3

import argparse
import collections
import logging
import os
import re
import subprocess
import sys
import yaml

log = logging.getLogger(__name__)

# The number of service IRQs for every Mellanox mlx5 PCI devices
NIC_SERVICE_IRQS = None
# Default path to the configuration file
CONFIG_PATH = "/etc/yc/tune-host-networking.yaml"

class EthtoolParams(object):
    QUEUE_NUMBER = "number"
    QUEUE_LENGTH = "length"
    GET_DICT = {QUEUE_NUMBER: "-l", QUEUE_LENGTH: "-g"}
    SET_DICT = {QUEUE_NUMBER: "-L", QUEUE_LENGTH: "-G"}

def parse_args():
    parser = argparse.ArgumentParser(description="Tune cpu affinity for network irqs")
    parser.add_argument("--config", default=CONFIG_PATH,
                      help="specify the path to the configuration file")
    parser.add_argument("--show", nargs="+", help="show current pinning for the interface(s)")

    args = parser.parse_args()

    return args


def load_config(path):
    with open(path, "r") as f:
        cfg = yaml.safe_load(f)
    return cfg

def pinning_to_str(pinning):
    return ", ".join(["{}->{}".format(i, c) for i, c in pinning.items()])

def read_queues_conf(iface, cmd):
    # Get nic configration from ethtool
    p = subprocess.check_output(["ethtool", EthtoolParams.GET_DICT[cmd], iface])
    output = p.decode("utf-8").strip().split("\n")

    # Parse ethtool output
    #
    # root@man1-c624-015:~# ethtool -l eth0vf0
    # Channel parameters for eth0vf0:
    # Pre-set maximums:
    # RX:\t\t0
    # TX:\t\t0
    # Other:\t\t512
    # Combined:\t\t4
    # Current hardware settings:
    # RX:\t\t0
    # TX:\t\t0
    # Other:\t\t0
    # Combined:\t\t4
    #
    # root@vla04-s5-25:~$ ethtool -g eth0
    # Ring parameters for eth0:
    # Pre-set maximums:
    # RX:\t\t8192
    # RX Mini:\t\t0
    # RX Jumbo:\t\t0
    # TX:\t\t8192
    # Current hardware settings:
    # RX:\t\t1024
    # RX Mini:\t\t0
    # RX Jumbo:\t\t0
    # TX:\t\t1024

    conf = {}
    section = ""
    for line in output[1:]:
        if line.endswith(":"):  # Here is new section: string ending on ':'
            # Parse only known sections, skip others
            if "maximums" in line:
                section = "max"
            elif "Current" in line:
                section = "current"
            else:
                section = ""  # Unknown section - skip
                continue
            conf[section] = {}
            continue

        if not section:  # Skip lines if known section is not found yet
            continue

        # Parse values
        words = line.split(":")
        if len(words) != 2:  # Want to work only with simple key:value pair
            continue
        key = words[0].strip(" \t")
        value = words[1].strip(" \t")

        conf[section].update({key.lower() : int(value)})

    return conf


def set_queues_conf(iface, cmd, conf):
    if "current" in conf:
        params = []
        current = conf["current"]
        for k in current:
            params.append(k)
            params.append(str(current[k]))

        subprocess.check_output(["ethtool", EthtoolParams.SET_DICT[cmd], iface] + params)


def get_iface_tx_queues_length(iface):
    # Get iface configuration using sysfs
    tx_queue_len_path = os.path.join("/sys/class/net", iface, "tx_queue_len")
    with open(tx_queue_len_path) as f:
        tx_queue_len = int(f.read())

    return tx_queue_len


def set_iface_tx_queues_length(iface, length):
    # Set iface configuration using sysfs
    tx_queue_len_path = os.path.join("/sys/class/net", iface, "tx_queue_len")
    with open(tx_queue_len_path, "w") as f:
        f.write(str(length))


def configure_number_nic_queues(iface, desired_number):
    # Check max available queues for the interface
    conf = read_queues_conf(iface, EthtoolParams.QUEUE_NUMBER)

    max_combined = conf["max"]["combined"]
    if not max_combined:
        raise ValueError("No combined rx/tx queues for {}".format(iface))

    # Set needed number of queues for RSS balancing per available cores
    rss_queues_to_set = min(max_combined, desired_number)

    cur_combined = 0
    try:
        cur_combined = conf["current"]["combined"]
    except KeyError:
        pass # it's ok if it is not configured

    # Don't change the number of queues if it is already set. In this case ethtool raises exception.
    if rss_queues_to_set != cur_combined:
        conf_to_change = {"current" : {"combined" : rss_queues_to_set}}
        set_queues_conf(iface, EthtoolParams.QUEUE_NUMBER, conf_to_change)

    log.debug("Set {}'s queues to {}".format(iface, rss_queues_to_set))

    return rss_queues_to_set

def configure_length_nic_queues(iface, desired_rx = None, desired_tx = None):
    # Check max available lengths queues for the interface
    conf = read_queues_conf(iface, EthtoolParams.QUEUE_LENGTH)

    max_rx = conf["max"]["rx"]
    max_tx = conf["max"]["tx"]

    cur_rx = conf["current"]["rx"]
    cur_tx = conf["current"]["tx"]

    conf_lengths = {}
    rx_to_set = cur_rx
    tx_to_set = cur_tx
    if desired_rx is not None:
        rx_to_set = min(max_rx, desired_rx)
        if rx_to_set != cur_rx:
            conf_lengths["rx"] = rx_to_set
    if desired_tx is not None:
        tx_to_set = min(max_tx, desired_tx)
        if tx_to_set != cur_tx:
            conf_lengths["tx"] = tx_to_set

    # Don't change settings if they are already set. In this case ethtool raises exception.
    if conf_lengths:
        conf_to_change = {"current" : conf_lengths}
        set_queues_conf(iface, EthtoolParams.QUEUE_LENGTH, conf_to_change)
        log.debug("Set {}'s NIC queues to {}. Other settings might be already set.".format(iface, str(conf_lengths)))

    return rx_to_set, tx_to_set


def get_iface_irqs(iface):
    # Get IRQs list for the interface
    iface_irqs_path = os.path.join("/sys/class/net", iface, "device/msi_irqs/")
    if not os.path.exists(iface_irqs_path):
        raise FileNotFoundError("The interface {} doesn't support MSI-X IRQs".format(iface))

    iface_irqs = os.listdir(iface_irqs_path)
    iface_irqs.sort()

    # Filter IRQs: an IRQ may not be used in /proc/interrupts by iterface
    filtered_iface_irqs = []
    with open("/proc/interrupts") as f:
        for line in f:
            irq = line.split(":")[0].strip()
            if irq.isdigit() and irq in iface_irqs:
                filtered_iface_irqs.append(irq)

    return filtered_iface_irqs


def set_irq_affinity(irq, cores):
    irq_smp_affinity_path = os.path.join("/proc/irq", irq, "smp_affinity_list")
    with open(irq_smp_affinity_path, "w") as f:
        f.write(cores)


def pin_irqs_to_cores(irqs, cores):
    if not (len(irqs[NIC_SERVICE_IRQS:]) <= len(cores)):
        raise ValueError("Not enough cores to pin all available queues' irqs: irqs={}, cores={}".format(irqs[NIC_SERVICE_IRQS:], len(cores)))

    pinning = collections.OrderedDict()

    # Distribute IRQs across the available cores:
    # 1. Pin first service IRQ lines to 1st core sice they are services (pages, cmd, async, fault)
    # 2. The rest IRQ lines (each next line is responsible for the correspoding queue)
    #    distributes equally across all cores starting from 1st core.
    # Overall, the 1st core will work with NIC_SERVICE_IRQS+1 lines assigned, the rest - with one.

    # Example of IRQs lines (from /proc/interrupts):
    # mlx5_pages_eq@pci:0000:03:08.3
    # mlx5_cmd_eq@pci:0000:03:08.3
    # mlx5_async_eq@pci:0000:03:00.2
    # mlx5_page_fault_eq@pci:0000:03:
    # mlx5_comp0@pci:0000:03:08.3 eth0vf0-0
    # mlx5_comp1@pci:0000:03:08.3 eth0vf0-1
    # mlx5_comp2@pci:0000:03:08.3 eth0vf0-2
    # mlx5_comp3@pci:0000:03:08.3 eth0vf0-3

    # Pin first 4 command IRQ lines to 1st core
    for irq in irqs[:NIC_SERVICE_IRQS]:
        core = cores[0]
        set_irq_affinity(irq, core)
        pinning[irq] = core

    # Pin each queue IRQ line to a new core
    # Note, there are enough cores to pin all irqs (see the pre-condition)
    for irq, core in zip(irqs[NIC_SERVICE_IRQS:], cores):
        set_irq_affinity(irq, core)
        pinning[irq] = core

    return pinning

def set_same_cpu_affinity(irqs, affinity):
    for irq in irqs:
        set_irq_affinity(irq, affinity)


def get_current_irqs_cores_pinning(iface):
    pinning = collections.OrderedDict()
    conf = read_queues_conf(iface, EthtoolParams.QUEUE_NUMBER)
    cur_queues = conf["current"]["combined"]
    irqs = get_iface_irqs(iface)
    for irq in irqs[:NIC_SERVICE_IRQS+cur_queues]:
        irq_smp_affinity_path = os.path.join("/proc/irq", irq, "smp_affinity_list")
        with open(irq_smp_affinity_path, "r") as f:
            core = f.readline().rstrip("\n")
            pinning[irq] = core
    return pinning

def check_irqs_cores_pinning(pinning):
    ok = True
    for irq in pinning:
        irq_smp_affinity_path = os.path.join("/proc/irq", irq, "smp_affinity_list")
        with open(irq_smp_affinity_path, "r") as f:
            set_core = f.readline().rstrip("\n")
            if set_core != pinning[irq]:
                log.warning("Checking IRQ {} pinning failed: current core - {}, expected core - {}".format(irq, set_core, pinning[irq]))
                ok = False
    return ok


def configure_network_cpu_affinity(iface, cores):
    """The function automatically configures network irqs:
    1. Set the number of NIC queues equal to the number of available cores.
    2. For each queue the approptiate IRQ line is pinned to the dedicated core.
    """
    if not (iface and cores):
        raise ValueError("No passed cores or interface name to isolate network IRQs: iface={}, cores={}".format(iface, cores))

    log.info("Isolating IRQs for '{}' across cores [{}] ... ".format(iface,",".join(cores)))

    n_queues = configure_number_nic_queues(iface, len(cores))

    irqs = get_iface_irqs(iface)

    # OS reserves IRQs for all available queues on the interface.
    # When we change the number of queues, OS doesn't change binding in /proc/interrupts.
    # OS just removes the interface's name from the approptiate IRQ line in /proc/interrupts
    # Thus we don't need to configure more than NIC_SERVICE_IRQS service IRQs and the number of IRQs per configured queues - so skip the rest IRQs
    irqs_to_pin = irqs[:NIC_SERVICE_IRQS+n_queues]

    pinning = pin_irqs_to_cores(irqs_to_pin, cores)

    log.info("Pinning for {} is [{}]".format(iface, pinning_to_str(pinning)))

    # Pinning might be unsuccessful if there is no more space in the IRQ APIC for the desired core.
    # TODO: it should be RuntimeError() since the service is not tuned performance correctly
    check_irqs_cores_pinning(pinning)


def set_default_network_cpu_affinity(iface, cores):
    """The function sets provided cores affinity mask to all iface related IRQ:
    1. Set the number of NIC queues equal to the number of given.
    2. Just set the same dedicated affinity mask for all configured queues.
    Note,
       - there is no guarantee that the queues are pinned each to its own core,
         configuring queues without pinning just should make this settings better,
       - the 'cores' must be in the format /proc/irq/*/smp_affinity_list.
    """
    if not (iface and cores):
        raise ValueError("No passed cores or interface name to isolate network IRQs: iface={}, cores={}".format(iface, cores))

    log.info("Setting default affinity mask for '{}' with [{}] ...".format(iface,",".join(cores)))

    configure_number_nic_queues(iface, len(cores))

    irqs = get_iface_irqs(iface)

    # We changed the number of queues for the interface, but we still want to clean pinning
    # for all queues in order to reset setup
    set_same_cpu_affinity(irqs, ",".join(cores))

    log.info("Set for {} the same affinity mask [{}] on all irqs [{}]".format(iface, ",".join(cores), ", ".join(irqs)))

def set_nic_queues_lengths_to_max(iface):
    """The function sets vhost's VF rx/tx queues lengths to max available values.
    """
    if not iface:
        raise ValueError("No passed nic interface name: iface={}".format(iface))

    log.info("Setting {}'s NIC rx/tx queues lengths to max ...".format(iface))

    rx_length, tx_length = configure_length_nic_queues(iface, desired_rx = sys.maxsize, desired_tx = sys.maxsize)

    log.info("New {}'s NIC queues lenghts are 'rx = {}, tx = {}'".format(iface, rx_length, tx_length))

def set_iface_transmit_queue_length(iface, desired_len):
    """ The function simply sets requested tx queue length for the interface
    """
    if not (iface and desired_len ):
        raise ValueError("No passed iface or tx queue length: iface={}, txqueuelen={}".format(iface, desired_len))

    log.info("Setting {}'s transmit queue length to {} ...".format(iface, desired_len))

    set_iface_tx_queues_length(iface, desired_len)

    new_length = get_iface_tx_queues_length(iface)

    if new_length == desired_len:
        log.info("New transmit queue length for {} is {}".format(iface, new_length))
    else:
        log.warning("Transmit queue length for {} didn't set correctly: desired = {}, current = {}".format(iface, desired_len, new_length))


def run_isolate_cpu(name):
    # The external script isolate_cpu.py is responsible for isolating cores for different services
    # > isolate_cpu.py show --type network --string
    # > 0, 28, 1, 29
    p = subprocess.check_output(["isolate_cpu.py", "show", "--type", name, "--string"])
    output = p.decode("utf-8").replace(" ","").strip("\n").split(",")
    return output


def configure_vhost_offloads(iface, offloads):
    """Turns eth0vf0 offload features on or off (depending on value provided by offloads)"""
    current_offloads = _toggle_iface_features(iface, ["ethtool", "-k", iface])
    for offload, target_status in offloads.items():
        current_status = current_offloads.get(offload, None)
        if current_status is None:
            log.warning("Offload %r is not supported", offload)
            continue
        if current_status == target_status:
            log.info("Offload %r is already in correct state", offload)
            continue

        toggled_offloads = _toggle_iface_features(iface, ["ethtool", "-K", iface, offload,
                                                          "on" if target_status else "off"])
        for toggled_offload, toggled_status in toggled_offloads.items():
            log.info("Offload %r is %s (by offload %r)", toggled_offload,
                     "disabled" if not toggled_status else "enabled", offload)


def _toggle_iface_features(iface, command):
    """Either runs ethtool -K to set offload feature or ethtool -k to get current status
    and parses output. Returns dict containing supported features and their states."""
    regex = re.compile(r"\s*([a-z-]+): (off|on)\s?(\[.*\])?")

    p = subprocess.check_output(command)
    output = p.decode("utf-8")

    offloads = {}
    for line in output.splitlines():
        m = regex.match(line)
        if not m:
            continue

        feature, status, _ = m.groups()
        offloads[feature] = (status == "on")

    return offloads


def main():
    global NIC_SERVICE_IRQS

    args = parse_args()

    cfg = load_config(args.config)

    NIC_SERVICE_IRQS = cfg["nic_service_irq"]

    # Show current pinning
    if args.show:
        for iface in args.show:
            pinning = get_current_irqs_cores_pinning(iface)
            log.info("Pinning for {} is [{}]".format(iface, pinning_to_str(pinning)))
        return

    # Setup irq pinning
    system_ifaces = cfg["system_interfaces"]
    vhost_iface = cfg["vhost_interface"]

    try:
        # Configure system interfaces by setting the same default irq affinity mask to all their irqs
        system_cores = run_isolate_cpu("irqaffinity")
        for iface in system_ifaces:
            set_default_network_cpu_affinity(iface, system_cores)

        # Tune the vhost interface by pinning the interface irqs to the isolated cpus
        network_cores = run_isolate_cpu("network")
        configure_network_cpu_affinity(vhost_iface, network_cores)

        # Increase NIC rx/tx queues' lengths for the all interfaces
        for iface in [vhost_iface] + system_ifaces:
            set_nic_queues_lengths_to_max(iface)

        # Increase transmit queue length for the all interfaces
        tx_queue_len = cfg['tx_queue_len']
        for iface in [vhost_iface] + system_ifaces:
            set_iface_transmit_queue_length(iface, tx_queue_len)

        configure_vhost_offloads(vhost_iface, cfg.get("vhost_offloads", {}))
    except (ValueError, RuntimeError, FileNotFoundError) as e:
        log.error(str(e))

if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG, format="%(levelname)s: %(message)s")
    main()

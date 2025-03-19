#!/usr/bin/env python3
import argparse
import json
import logging
import os
import subprocess


# Some interfaces do not support VF trusts
NON_TRUST_DEVICES = {
    '15b3:1007',  # Mellanox CX3-Pro
    '15b3:1003',  # Mellanox CX3
    '8086:1521',  # Intel I350
}
NON_MIN_TX_RATE_DEVICES = {
    '15b3:1007',  # Mellanox CX3-Pro
    '15b3:1003',  # Mellanox CX3
    '8086:1521',  # Intel I350
    '8086:10fb',  # Intel 82599ES
}
NON_MAX_TX_RATE_DEVICES = {
    '15b3:1007',  # Mellanox CX3-Pro
    '15b3:1003',  # Mellanox CX3
    '8086:1521',  # Intel I350
    '8086:10fb',  # Intel 82599ES
}


def log_init(level, logprefix='config-vf'):
    logger = get_logger(logprefix)
    handler = logging.StreamHandler()
    formatter = logging.Formatter(
        '%(asctime)s %(name)-18s %(levelname)-8s %(message)s')
    handler.setFormatter(formatter)
    logger.addHandler(handler)
    logger.setLevel(level)


def get_logger(logprefix='config-vf'):
    return logging.getLogger(logprefix)


def parse_args():
    """Parses comand line arguments."""
    parser = argparse.ArgumentParser(
        description="Helper tool to configure SRI-OV VFs")
    parser.add_argument("-c",
                        dest="config",
                        default="/etc/yc/yc-network-profiles.json")
    parser.add_argument("-p", "--pf",
                        dest="pf",
                        help="Name of the PF interface", required=True)
    parser.add_argument("-v", "--vf",
                        dest="vf",
                        help="Number of the VF interface",
                        type=int,
                        required=True)
    parser.add_argument("profile",
                        help="Name of the profile in the config file")
    return parser.parse_args()


def _vendor_device(pf_name):
    eth_path = os.path.realpath("/sys/class/net/{}".format(pf_name))
    dev_path = os.path.normpath(os.path.join(eth_path, '../..'))
    with open(os.path.join(dev_path, 'vendor')) as f:
        vendor = f.readline().strip()[2:]  # without '0x'
    with open(os.path.join(dev_path, 'device')) as f:
        device = f.readline().strip()[2:]  # without '0x'
    return "{}:{}".format(vendor, device)


def config_vf(pf, vfnum, profile):
    args = ["sudo", "ip", "link", "set", "dev", pf, "vf", str(vfnum)]

    vendor_device = _vendor_device(pf)
    logger = get_logger()

    # NOTE(kzaitsev): If we do not specify any param: reset it to 0,
    # so that params are reset if we reuse the same VF for a different net.
    # It is also ok to set 'vlan 0 qos 7' for example
    args.extend(["vlan", str(profile.get("vlan", 0))])
    args.extend(["qos", str(profile.get("qos", 0))])

    if vendor_device in NON_MAX_TX_RATE_DEVICES:
        logger.warning(
            "Ignoring 'max_tx_rate' parameter "
            "{} doesn't support it".format(pf))
    else:
        args.extend(["max_tx_rate", str(profile.get("max_tx_rate", 0))])

    if vendor_device in NON_MIN_TX_RATE_DEVICES:
        logger.warning(
            "Ignoring 'min_tx_rate' parameter "
            "{} doesn't support it".format(pf))
    else:
        args.extend(["min_tx_rate", str(profile.get("min_tx_rate", 0))])

    if vendor_device in NON_TRUST_DEVICES:
        logger.warning(
            "Ignoring 'trust' parameter {} doesn't support it".format(pf))
    else:
        args.extend(["trust", str(profile.get("trust", "on"))])
    subprocess.check_call(args)


def main():
    cfg = parse_args()
    log_init(logging.INFO)
    with open(cfg.config) as f:
        available_profiles = json.load(f)
    if cfg.profile not in available_profiles:
        raise ValueError("No profile for {}. Available profiles {}".format(
            cfg.profile, list(available_profiles.keys())))
    profile = available_profiles[cfg.profile]
    config_vf(cfg.pf, cfg.vf, profile)


if __name__ == '__main__':
    main()

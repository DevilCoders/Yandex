#!/usr/bin/env python3

import argparse
import json
import logging
import os
import sys
import urllib.request

STATUS_OK = 0
STATUS_WARN = 1
STATUS_CRIT = 2
NETMON_RUN_DIR = "/var/lib/netmon"
NETMON_STAT_PORT = 8090  # TODO(k-zaitsev): read from config

log = logging.getLogger("netmon-agent-health")


def report_and_exit(status, desc):
    if status != STATUS_OK:
        desc += " See https://nda.ya.ru/3UZQVu"
    print("{};{}".format(status, desc))
    sys.exit(0)


def check_running():
    pidfile = os.path.join(NETMON_RUN_DIR, "netmon-agent.pid")
    try:
        with open(pidfile) as f:
            pid = f.read().strip()
    except OSError as e:
        log.exception("Can't open pid file")
        report_and_exit(STATUS_CRIT, "Netmon agent seems down. Can't open pid file ({}).".format(e))

    cmdline = os.path.join("/proc", pid, "cmdline")
    try:
        with open(cmdline) as f:
            contents = f.read()
    except OSError:
        log.exception("Can't read file {}".format(cmdline))
        report_and_exit(STATUS_CRIT, "Netmon agent seems down. Can't read file {}.".format(cmdline))
    if 'netmon-agent' not in contents:
        log.error("Couldn't find 'netmon-agent' in {}, {}".format(contents, cmdline))
        report_and_exit(STATUS_CRIT, "Netmon agent seems down. {} points to a different process".format(pidfile))


def check_stats():
    url = "http://localhost:{}/stats".format(NETMON_STAT_PORT)
    try:
        data = urllib.request.urlopen(url).read().decode()
    except urllib.request.URLError as e:
        log.exception("Can't read stats on url {}".format(url))
        report_and_exit(STATUS_CRIT, "Can't get agent's stats. Got {}".format(e))
    try:
        parsed_data = json.loads(data)
    except json.JSONDecoder:
        log.exception("Can't parse stats on url {}".format(url))
        report_and_exit(STATUS_CRIT, "Can't parse agent's stats.")
    if not parsed_data:
        topo_file = os.path.join(NETMON_RUN_DIR, 'topology.msgpack.gz')
        topo_file_exists = 'present' if os.path.exists(topo_file) else 'absent'

        report_and_exit(STATUS_CRIT, "Agent's stats are empty. It is not sending/recieving packets. {} is {}".format(
            topo_file, topo_file_exists))

    # TODO(k-zaitsev): Save echo_received_probes_summ & echo_sent_probes_summ values to a file, check that they're
    # increasing, report yellow if they're not


def main():
    parser = argparse.ArgumentParser(description="Netmon agent health check")
    parser.add_argument("--debug", action="store_true")
    args = parser.parse_args()

    log.addHandler(logging.StreamHandler())
    log.setLevel(logging.DEBUG)
    log.disabled = not args.debug

    check_running()
    check_stats()
    report_and_exit(STATUS_OK, "Netmon agent is running and seems healthy.")


if __name__ == "__main__":
    main()

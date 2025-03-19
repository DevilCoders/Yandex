#!/usr/bin/env python3
import argparse
import json
import logging
import os
import re
import subprocess
import sys
import time

import lib

def parse(content):
    items = {}
    liter = iter(content.splitlines())
    try:
        while True:
            line = next(liter)
            if line.startswith("Current temperature"):
                [_, val] = line.split(":")
                items['Temperature'] = val.strip()
            elif "System Power Supply Status" in line:
                line = next(liter)
                while True:
                    line = next(liter)
                    logging.debug("parse psu: %s", line)
                    if line.startswith('='):
                        break
                    m = re.search('^PSU(\d+) Information', line)
                    if m:
                        # почему-то PSU Information нумеруется с 0, а PSU Fan - с 1
                        # приводим к началу к 1, т.к. в соломоне уже есть такие метрики (из UFM)
                        psu_number = int(m.group(1))+1
                        psu_status = 0
                        psu_fan_status = 0
                        while True:
                            line = next(liter)
                            logging.debug("parse psu#%s: %s", psu_number, line)
                            if not ':' in line:
                                items["psu%s_status" % psu_number] = psu_status
                                items["psu%s_fan_status" % psu_number] = psu_fan_status
                                break
                            [name, val] = line.split(":")
                            name = name.strip()
                            val = val.strip()
                            
                            logging.debug("var %s", [name, val])
                            if name == 'DC status' and val == 'DC Power OK':
                                psu_status = 1
                            if name == 'PSU fan status' and val == 'PSU fan OK':
                                psu_fan_status = 1

            elif line.startswith('Fan ') or line.startswith('PSU Fan '):
                m = re.search('^(PSU)? ?Fan #(\d+) speed.*: (\d+)$', line)
                if m:
                    if m.group(1):
                        fan = '%s%s_fan_rpm' % (m.group(1), m.group(2))
                    else:
                        fan = 'fan%s_rpm' % (m.group(2))
                    items[fan.lower()] = m.group(3)
    except StopIteration:
        return items


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", help="config file", action="store")
    parser.add_argument("--debug", help="enable debug", action="store_true")
    parser.add_argument("--ib-diag-file", help="parse file instead calling ibdiagnet2", action="store")
    parser.add_argument("--switch-info-file", help="parse file (use %%s as placeholder for switch guid) instead calling manage_the_unmanaged", action="store")
    parser.add_argument("--fqdn", help="use this FQDN instead local host name", action="store")

    options = parser.parse_args()
    level = logging.WARNING
    if options.debug:
        level = logging.DEBUG
    logging.basicConfig(level=level,
                        format="%(asctime)s - %(filename)s:%(lineno)d - %(funcName)s() - %(levelname)s - %(message)s")

    logging.info(options)
    fqdn = options.fqdn if options.fqdn else lib.my_fqdn()
    config = lib.read_config(options.config)
    token = config['auth']['invapi_token']
    ufm_hosts_names = lib.get_ufm_hosts_for_cluster_with_host_cached(fqdn, token)
    cluster_name = ufm_hosts_names[fqdn]["cluster"].lower()

    if not options.ib_diag_file:
        content = lib.read_ibdiagnet_db(cachettl=60)
    else:
        with open(options.ib_diag_file) as f:
            content = f.read()

    sections = lib.split_to_sections(content)
    (switch_to_descr, hca_to_descr) = lib.collect_nodes(sections)
    switch_guids = [ switch['NodeGUID'] for switch in lib.parse_section(sections['SWITCHES'])]
    logging.debug("Found switches: %s", switch_guids)

    metrics = []
    for guid in switch_guids:
        if not options.switch_info_file:
            logging.debug("inspect #%s", guid)
            result = subprocess.run(["env", "IBPATH=/opt/ib-mon/wrapper/", "sudo", "manage_the_unmanaged", "--guid", guid], stdout=subprocess.PIPE)
            out = result.stdout.decode().strip()
            logging.debug("exit code %d", result.returncode)
        else:
            try:
                filename = options.switch_info_file % guid
            except TypeError:
                filename = options.switch_info_file
                
            logging.debug("read #%s status from file %s", guid, filename)
            with open(filename) as f:
                out = f.read()

        items = parse(out)

        # для совместимости с mondata/solomon/IB_system.py, оперирующем метриками от UFM
        ufm_compatible = {}
        for item in items:
            m = re.search("^psu(\d+)_status$", item)

            if m:
                ufm_compatible["PS-%s" % m.group(1)] = items[item]
                continue

            m = re.search("^fan(\d+)_rpm$", item)
            if m:
                ufm_compatible["FAN-%s" % m.group(1)] = 1 if int(items[item]) > 1000 else 0
                continue

        items.update(ufm_compatible)

        logging.debug(items)
        tags = {
                "host": switch_to_descr[guid],
                "_cluster": cluster_name,
            }
        metrics.append({"tags": tags, "fields": items})

    timestamp = int(time.time())
    f = lib.to_influx_fmt(metrics, "infiniband", timestamp)
    for line in f:
        print(line)


#!/usr/bin/env python3
import lib
import argparse
import logging
import os
import sys
import json
import time

def get_temperature_of_switches(file, user, password):
    systems = collect_data(file, "systems", user, password)
    temperature = {}
    for host in systems:
        if host["type"].lower() != "switch":
            continue
        if host["description"].find(" ") >= 0:
            continue
        logging.debug("* temperature: %s", host["temperature"])
        try:
            temperature[host["description"]] = {"Temperature": int(host["temperature"])}
        except ValueError as e:
            # temperature: N/A
            pass

    return temperature


def get_switches_modules(file, user, password):
    all_modules = collect_data(file, "modules", user, password)
    modules = {}
    supported_types = ["PS", "FAN"]
    for dev in all_modules:
        if dev["device_type"].lower() != "switch" or dev["type"].upper() not in supported_types:
            continue
        if dev["device_name"].find(" ") >= 0:
            continue

        name = dev['description'].replace(" ", "")
        status = 1 if dev['status'].upper() == "OK" else 0

        if dev["device_name"] not in modules:
            modules[dev["device_name"]] = {}

        values = modules[dev["device_name"]]
        values[name] = status
    return modules


def collect_data(file, command, user, password):
    data = {}
    if not file:
        logging.info("using file '%s'", file)

        if command == "systems":
            url = "https://localhost/ufmRest/resources/systems?type=switch"
        elif command == "modules":
            url = "https://localhost/ufmRest/resources/modules"

        data = lib.get_url_with_auth_and_seflsigined_cert(url, user, password)
    else:
        logging.info("using file '%s'", file)
        with open(file) as f:
            data = f.read()
    logging.info("data", data)
    return json.loads(data)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", help="config file", action="store")
    parser.add_argument("--debug", help="enable debug", action="store_true")
    parser.add_argument("--file", help="parse file", action="store")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--temperature", help="get temperature", action="store_true")
    group.add_argument("--modules", help="get modules (PS and FAN)", action="store_true")
    parser.add_argument("--fqdn", help="use this FQDN instead local host name", action="store")

    options = parser.parse_args()
    level = logging.WARNING
    if options.debug:
        level = logging.DEBUG
    logging.basicConfig(level=level,
                        format="%(asctime)s - %(filename)s:%(lineno)d - %(funcName)s() - %(levelname)s - %(message)s")

    basedir = os.path.dirname(os.path.realpath(__file__))
    vardir = os.path.join(basedir, "var")

    logging.info(options)
    fqdn = options.fqdn if options.fqdn else lib.my_fqdn()
    config = lib.read_config(options.config)
    token = config['auth']['invapi_token']
    ufm_hosts_names = lib.get_ufm_hosts_for_cluster_with_host_cached(fqdn, token, vardir)
    cluster_name = ufm_hosts_names[fqdn]["cluster"].lower()

    with open(os.path.join(basedir, "auth.conf")) as f:
        [user, _sep, password] = f.read().split(" ")
        user = user.strip()
        password = password.strip()

    if options.temperature:
        data = get_temperature_of_switches(options.file, user, password)
    elif options.modules:
        data = get_switches_modules(options.file, user, password)
    else:
        print("You should set --temperature or --modules", file=sys.stderr)
        sys.exit(1)

    res = []
    for host, fields in data.items():
        tags = {
                "host": host.lower(),
                "_cluster": cluster_name,
            }
        res.append({"tags": tags, "fields": fields})

    timestamp = int(time.time())
    f = lib.to_influx_fmt(res, "infiniband", timestamp)
    for line in f:
        print(line)



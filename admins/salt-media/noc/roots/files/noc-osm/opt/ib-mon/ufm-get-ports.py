#!/usr/bin/env python3

import logging
import os.path
import argparse
import re
import json
import sys

import lib

# TODO:
# - get data from ibdiagnet


def main(file):
    if not file:
        content = lib.read_ibdiagnet_db(cachettl=60)
    else:
        with open(file) as f:
            content = f.read()

    sections = lib.split_to_sections(content)

    (links, switch_to_descr, hca_to_descr, port_properties) = lib.collect_links_and_nodes(sections)
    devices = {}
    for (from_dev, from_port), (to_dev, to_port) in links.items():
        if from_dev in switch_to_descr:
            if from_dev not in devices:
                devices[from_dev] = {
                    "name": switch_to_descr[from_dev],
                    "ports": []
                }

            if to_dev in switch_to_descr:
                neighbor_name = switch_to_descr[to_dev]
            elif to_dev in hca_to_descr:
                neighbor_name = hca_to_descr[to_dev]
            else:
                neighbor_name = ""

            devices[from_dev]["ports"].append({
                "num": from_port,
                "neighbor": {
                    "num": to_port,
                    "name": neighbor_name,
                },
                "props": port_properties[(from_dev, from_port)],
            })


    return list(devices.values())


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--debug", help="enable debug", action="store_true")
    parser.add_argument("--file", help="parse file", action="store")
    parser.add_argument("--host", help="get ports for host", action="store")
    options = parser.parse_args()
    level = logging.WARNING
    if options.debug:
        level = logging.DEBUG
    logging.basicConfig(level=level,
                        format="%(asctime)s - %(filename)s:%(lineno)d - %(funcName)s() - %(levelname)s - %(message)s")


    devices = main(options.file)

    if options.host:
        name = options.host.lower()
        for dev in devices:
            if dev['name'].lower() == name:
                print(json.dumps(dev))
                sys.exit(0)

        print("Host '%s' not found" % (options.host), file=sys.stderr)
        sys.exit(1)
    else:
        print(json.dumps(devices))
        sys.exit(0)


#!/usr/bin/env python3

import logging
import os.path
import os
import argparse
import time

import lib

from lib import get_port_state, get_port_phy_state, get_link_mult, get_speed, PM_INFO_TYPES, PORTS_TYPES

MEASUREMENT_NAME = "infiniband"

LINKEYE_MAPPING = {
    # PortXmitData/PortRcvData - the total number of data octets,
    # divided by 4, transmitted/received on all VLs from the port.
    # We should convert it to bits/sec
    "tx": lambda x: x["PortXmitData"]*4,
    "rx": lambda x: x["PortRcvData"]*4,
    "rx_drop": "PortBufferOverrunErrors",
    "tx_drop": "PortXmitDiscards",
    "rx_errs": lambda x: x["PortRcvErrors"] + x["SymbolErrorCounter"],
    "tx_errs": lambda x: 0, # sum of some Errors?...
    "tx_unicast": "PortUniCastRcvPkts",
    "rx_unicast": "PortUniCastXmitPkts",
    "speed": "PortSpeed",
    "rx_packets": "PortRcvPkts",
    "tx_packets": "PortXmitPkts",
    #"rx_nunicast": "",
    #"tx_nunicast": "",
    "flap": "LinkDownedCounter",
    "admin_status": lambda x: x["PortStateSNMP"],
    "oper_status": lambda x: x["PortStateSNMP"],
}


def main(file, ufm_hosts_names, faketime, fqdn):
    if not faketime:
        timestamp = int(time.time())
    else:
        timestamp = 4 # this is fake static random time

    if not file:
        content = lib.read_ibdiagnet_db()
    else:
        with open(file) as f:
            content = f.read()

    sections = lib.split_to_sections(content)
    (ports_res, ports_direction) = prepare(sections, ufm_hosts_names)

    linkeye_res = []
    for keys, values in ports_res.items():
        ((_, host), (_, port)) = keys

        v = {}
        for k in LINKEYE_MAPPING:
            repl = LINKEYE_MAPPING[k]
            try:
                if callable(repl):
                    v[k] = repl(values)
                else:
                    v[k] = values[repl]
                if v[k] < 0:
                    v[k]=0
            except KeyError as e:
                # when port flapping some keys (i.e. PortXmitData) does not exists in ibdiagnet
                # do not send
                pass

        keys = {"ifname": port, "host": host.lower(), "ifalias": port, "telegraf_route": "linkeye", }
        linkeye_res.append({"tags": keys, "fields": v})

    infiniband_res = []
    for keys, values in ports_res.items():
        tags = dict(keys)
        tags["_cluster"] = ufm_hosts_names[fqdn]["cluster"].lower()
        if keys in ports_direction:
            tags["direction"] = ports_direction[keys];
        infiniband_res.append({"tags": tags, "fields": values})

    f = lib.to_influx_fmt(linkeye_res, "network", timestamp)
    for line in f:
        print(line)

    f = lib.to_influx_fmt(infiniband_res, MEASUREMENT_NAME, timestamp)
    for line in f:
        print(line)


def prepare(data, ufm_hosts_names):
    (links, switch_to_descr, hca_to_descr, _) = lib.collect_links_and_nodes(data)
    ports_res = {}
    ports_direction = {}

    # PORTS parse
    for port in lib.parse_section(data["PORTS"]):
        if port["PortNum"] == "0":
            continue
        if port["NodeGuid"] == port["PortGuid"]:
            if port["NodeGuid"] in switch_to_descr:
                # links from switches to HCA
                host_descr = switch_to_descr[port["NodeGuid"]]
                port_num = port["PortNum"]
            elif port["NodeGuid"] in hca_to_descr and hca_to_descr[port["NodeGuid"]].split(" ")[0] in ufm_hosts_names:
                [host_descr, port_num] = hca_to_descr[port["NodeGuid"]].split(" ")
            else:
                # HCA port
                continue

            direction = "n/a"
            link_from = (port["NodeGuid"], port["PortNum"])
            if link_from in links:
                (to_node, _to_port) = links[link_from]
                if to_node in switch_to_descr:
                    direction = "to-switch" #-%s" % host_descr[to_node]
                elif to_node in hca_to_descr and hca_to_descr[to_node].split(" ")[0] in ufm_hosts_names:
                    direction = "to-ufm_%s" % hca_to_descr[to_node].replace(" ", "%")
                elif to_node in hca_to_descr:
                    direction = "to-hca_%s" % hca_to_descr[to_node].replace(" ", "%")

            keys = {"host": host_descr, "port": port_num,}

            values = port.copy()
            for del_attr in ["NodeGuid", "PortGuid", "PortNum"]:
                values.pop(del_attr)
            values = lib.parse_values(values, PORTS_TYPES)
            #print(values, port)
            values["PortStateSNMP"] = get_port_state(values["PortState"])
            values["PortPhyStateSNMP"] = get_port_phy_state(values["PortPhyState"])
            values["PortSpeed"] = get_speed(values['LinkSpeedActive'], values['LinkWidthActive'])
            values["LinkWidthMultiplier"] = get_link_mult(values['LinkWidthActive'])

            key = tuple(sorted(keys.items()))
            ports_res[key] = values
            ports_direction[key] = direction
    # PM_INFO parse

    for port in lib.parse_section(data["PM_INFO"]):
        if port["NodeGUID"] == port["PortGUID"]:
            if port["NodeGUID"] in switch_to_descr:
                host_descr = switch_to_descr[port["NodeGUID"]]
                port_num = port["PortNumber"]
            elif port["NodeGUID"] in hca_to_descr and hca_to_descr[port["NodeGUID"]].split(" ")[0] in ufm_hosts_names:
                [host_descr, port_num] = hca_to_descr[port["NodeGUID"]].split(" ")
            else:
                continue
            keys = {"host": host_descr, "port": port_num}
            values = port.copy()
            for del_attr in ["NodeGUID", "PortGUID", "PortNumber"]:
                values.pop(del_attr)
            values = lib.parse_values(values, PM_INFO_TYPES)
            # print(keys, values)
            ports_res[tuple(sorted(keys.items()))].update(values)

    return (ports_res, ports_direction)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", help="config file", action="store")
    parser.add_argument("--debug", help="enable debug", action="store_true")
    parser.add_argument("--file", help="parse file", action="store")
    parser.add_argument("--fqdn", help="use this FQDN instead local host name", action="store")
    parser.add_argument("--faketime", help="use fake time", action="store_true")
    options = parser.parse_args()
    level = logging.WARNING
    if options.debug:
        level = logging.DEBUG
    logging.basicConfig(level=level,
                        format="%(asctime)s - %(filename)s:%(lineno)d - %(funcName)s() - %(levelname)s - %(message)s")

    fqdn = options.fqdn if options.fqdn else lib.my_fqdn()
    config = lib.read_config(options.config)
    token = config['auth']['invapi_token']
    ufm_hosts_names = lib.get_ufm_hosts_for_cluster_with_host_cached(fqdn, token)

    main(options.file, ufm_hosts_names, options.faketime, fqdn)


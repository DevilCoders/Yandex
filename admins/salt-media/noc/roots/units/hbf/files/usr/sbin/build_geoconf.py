#!/usr/bin/pynoc

import ipaddress
import sys
import logging
import time
import os

from textwrap import indent
import rtapi


DC_DEFAULT = "vla"
DC_MAP = {"Владимир": "vla",
          "Владимир AZ": "vlx",
          "Ивантеевка": "iva",
          "Сасово": "sas",
          "Мытищи": "myt",
          "Финляндия": "man",
          }


def get_rt_nets():
    rt = rtapi.RTAPI(auth_try_ssh=False)
    nets = {}
    for fam in ["ipv6net", "ipv4net"]:
        ipvnets = rt.call.scanRealmByText(fam, "{агрегат}")
        for net_id, net_data in ipvnets.items():
            tags = [t["tag"] for t in net_data["etags"].values()]
            tags += [t["tag"] for t in net_data["itags"].values()]
            for tag in tags:
                if tag in DC_MAP:
                    loc = DC_MAP[tag]
                    break
            else:
                loc = DC_DEFAULT
            nets[(net_data["ip"], net_data["mask"])] = loc
    res = {}

    for net, d in sorted(nets.items(), key=lambda x: x[0][1]):  # по маске сортировка
        net = ipaddress.ip_network("%s/%s" % net)
        included = False
        # проверим что для данной сети нет покрывающей сети с аналогичным loc
        for seen_net, seen_loc in res.items():
            seen_net = ipaddress.ip_network(seen_net)
            if net.version != seen_net.version:
                continue
            if (int(net.network_address) & int(seen_net.netmask) == int(seen_net.network_address)
                    and seen_net.prefixlen <= net.prefixlen):
                if d == seen_loc:
                    included = True
        if not included:
            res[net] = d

    return res


def render_config():
    data = list(get_rt_nets().items())
    data.sort(key=lambda x: (x[0].version, x[0]))
    res = "default\t%s;\n" % DC_DEFAULT
    res += "\n".join(["%s\t%s;" % (ip, geo) for (ip, geo) in data])
    res = "geo $geo {\n%s\n}" % indent(res, " " * 4)
    return res


if __name__ == "__main__":
    new_config = render_config()
    if len(new_config) < 1000:
        print("generated config is too small", file=sys.stderr)
        sys.exit(1)
    if len(sys.argv) > 1:
        target_file = sys.argv[1]
        new_config = new_config.strip()
        if os.path.isfile(target_file):
            lines = open(target_file, "r+").readlines()
        else:
            lines = []
        # сравниваем файл не считая первой динамической строки
        old_config = "".join(lines[1:]).strip()
        if old_config != new_config:
            with open(target_file, "w+") as f:
                logging.debug("update")
                f.write("# generated at %s by %s\n" % (time.strftime("%d-%m-%Y %H:%M:%S"), __file__))
                f.write(new_config)
                f.write("\n")
        else:
            logging.debug("keep")
    else:
        print(new_config)

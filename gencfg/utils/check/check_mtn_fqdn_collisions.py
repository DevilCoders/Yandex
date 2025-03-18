#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg

import ujson
from core.db import CURDB
from gaux.aux_colortext import red_text
from gaux.aux_decorators import gcdisable
from gaux.aux_hbf import generate_hbf_info

@gcdisable
def main():
    failed = False

    fqdns = dict()

    ignored_fqdn_versions = set([0, 1, 2])

    for group in CURDB.groups.get_groups():
        if group.card.properties["mtn_fqdn_version"] in ignored_fqdn_versions:
            continue

        for instance in group.get_instances():
            hbf_info = generate_hbf_info(group, instance)
            if "interfaces" not in hbf_info:
                continue

            for interface, interface_info in hbf_info["interfaces"].iteritems():
                mtn_hostname = interface_info["hostname"]
                if mtn_hostname not in fqdns:
                    fqdns[mtn_hostname] = list()
                entry = {
                    "group": repr(group),
                    "mtn_fqdn_version": group.card.properties["mtn_fqdn_version"],
                    "host": "{}:{}".format(instance.get_host_name(), instance.port)
                }
                fqdns[mtn_hostname].append(entry)

    collisions = {k: v for k, v in fqdns.iteritems() if len(v) > 1}

    if collisions:
        print red_text(ujson.dumps(collisions, indent=4, sort_keys=True))
        failed = True

if __name__ == '__main__':
    status = main()
    sys.exit(status)

#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB
from gaux.aux_decorators import gcdisable

@gcdisable
def main():
    db = CURDB
    all_search_hosts = set(db.groups.get_group("ALL_SEARCH").getHosts())

    invalid_hosts = [host for host in all_search_hosts if host.is_vm_guest()]
    if invalid_hosts:
        names = sorted([host.name for host in invalid_hosts])
        raise Exception("Found virtual host inside ALL_SEARCH which is forbidden! Please set nonsearch flag to their host groups! Hosts: %s" % ", ".join(names))


if __name__ == "__main__":
    main()

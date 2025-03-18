#!/skynet/python/bin/python

"""
    Check if we have dismounted machines in our group. If yes, exit with non-zero status.
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB
from gaux.aux_utils import retry_urlopen
from gaux.aux_colortext import red_text
from gaux.aux_reserve import UNSORTED_GROUP, PRENALIVKA_GROUP
from core.settings import SETTINGS


def get_bot_hosts():
    result = []

    data = retry_urlopen(3, SETTINGS.services.oops.rest.hosts.url)
    for line in data.strip().split('\n'):
        hostname = line.split('\t')[1]
        if hostname != '':
            result.append(hostname.lower())
    return result


def main():
    bot_hosts = get_bot_hosts()

    my_hosts = map(lambda x: x.name, filter(lambda x: not x.is_vm_guest(), CURDB.hosts.get_hosts()))

    ignore_hosts = []
    for groupname in [UNSORTED_GROUP, PRENALIVKA_GROUP]:
        ignore_hosts.extend(map(lambda x: x.name, CURDB.groups.get_group(groupname).getHosts()))

    result_hosts = set(my_hosts) - set(bot_hosts) - set(ignore_hosts)

    if len(result_hosts) > 0:
        print red_text(
            "Hosts <%s> already dismounted, but yet not removed from search groups" % (",".join(result_hosts)))
        return 1
    else:
        return 0


if __name__ == '__main__':
    status = main()

    sys.exit(status)

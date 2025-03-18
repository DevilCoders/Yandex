#!/skynet/python/bin/python

import inspect

import gencfg
import gaux.aux_utils
import core.hosts
import utils.pregen.update_hosts
from core.db import CURDB

HEAD = """#!/skynet/python/bin/python

import re
import urllib2
import os
import socket
import subprocess
import time
import random
import argparse
from pprint import pprint
import msgpack
import fcntl

"""

TAIL = """
# ===============================================================================
# main stuff
# ===============================================================================
def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-f', '--format', choices=('pretty', 'binary'), default='pretty')
    return parser.parse_args()

def main():
    args = parse_args()

    hwdata, warnings = detect_all()

    if args.format == 'pretty':
        pprint(hwdata)
    elif args.format == 'binary':
        print(msgpack.packb(hwdata))

if __name__ == '__main__':
    main()
"""


def _sl(obj):
    return ''.join(inspect.getsourcelines(obj)[0]) + '\n'


def main():
    result = HEAD

    result += _sl(gaux.aux_utils.run_command)
    result += _sl(utils.pregen.update_hosts.geturl)
    result += _sl(utils.pregen.update_hosts._get_primary_disk)
    result += _sl(utils.pregen.update_hosts.hivemind_hostname)
    result += _sl(utils.pregen.update_hosts.botqueryurl)
    result += _sl(utils.pregen.update_hosts.is_vm_guest)
    result += _sl(utils.pregen.update_hosts.LinuxDiskInfo)
    result += _sl(utils.pregen.update_hosts.GetStats)

    # add all detector funcs
    avail_funcs = filter(lambda x: x.startswith('detect_'), dir(utils.pregen.update_hosts))
    # filter non-obligatory elements
    avail_funcs = filter(
        lambda x: x not in ['detect_switch', 'detect_ipv4addr', 'detect_ipv6addr', 'detect_platform', ], avail_funcs)
    for funcname in avail_funcs:
        result += _sl(getattr(utils.pregen.update_hosts, funcname))

    # add main func
    cpu_models = map(lambda x: (x.fullname, x.model), CURDB.cpumodels.models.values())
    models_as_str = "{ %s }" % (", ".join(map(lambda (fullname, model): "'%s' : '%s'" % (fullname, model), cpu_models)))
    funcs_as_str = "{ %s }" % (", ".join(map(lambda x: "'%s' : %s" % (x.partition('_')[2], x), avail_funcs)))

    main_func = """def detect_all():
    stats_getter = GetStats(%(models)s, %(funcs)s, ignore_detect_fail = True)
    return stats_getter.run()
""" % {'models': models_as_str, 'funcs': funcs_as_str}

    result += main_func

    result += TAIL

    return result

#!./venv/venv/bin/python
"""Perform various actions with dnscache entities"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
sys.path.append('/skynet')

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt

import utils.pregen.update_hosts


class Dummy(object):
    __slots__ = ['params']

    def __init__(self, params):
        self.params = params


class EActions(object):
    UPDATE = "update"
    ALL = [UPDATE]


def get_parser():
    parser = ArgumentParserExt(description="Perform various actions with local dnscache")
    parser.add_argument("-a", "--action", type=str, default = EActions.UPDATE,
                        choices=EActions.ALL,
                        help="Obligatory. Action to execute")
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Optional. Verbose level (specify multiple times to get more verbose output)")

    return parser


def main(options):
    if options.action == EActions.UPDATE:
        all_hosts = [x.name for x in CURDB.hosts.get_hosts() if x.is_vm_guest()]
        already_in_cache_hosts = [x for x in all_hosts if CURDB.dnscache.has(x)]
        print 'Updating DnsCache: {} hosts total, {} already in dns cache'.format(len(all_hosts), len(already_in_cache_hosts))

        updated = 0  # number of hosts with ipv6addr updated/added first time
        failed = 0  # number of hosts with failed detection of addr
        for host in all_hosts:
            result = utils.pregen.update_hosts.detect_ipv6addr(Dummy({'custom_hostname': host}))

            # failed to detect
            if result == 'unknown':
                failed += 1
                continue

            # newly added host
            if not CURDB.dnscache.has(host):
                CURDB.dnscache.modify(host, result)
                updated += 1
                continue

            # host already in db
            if CURDB.dnscache.get(host) != result:
                CURDB.dnscache.modify(host, result)
                updated += 1
                continue

        print 'Updating DnsCache result: {} updated hosts, {} failed hosts'.format(updated, failed)
        CURDB.dnscache.update()
    else:
        raise Exception('Unknown action <{}>'.format(options.action))


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    main(options)

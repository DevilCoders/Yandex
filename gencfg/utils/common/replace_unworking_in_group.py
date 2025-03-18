#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
from collections import defaultdict

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types
from core.argparse.parser import ArgumentParserExt
from utils.common import replace_host2
from utils.pregen import check_alive_machines


class ResultType(object):
    def __init__(self):
        self.result = defaultdict(dict)


CHECKERS = {
    'sshport': 'ALL_UNWORKING',
}


def get_parser():
    DEFAULT_PERCENTS = 1.0
    DEFAULT_COMPARATOR = "mem=,power=,disk=,ssd=,queue-,dc-,location="

    parser = ArgumentParserExt(description="Find and replace all unworking machines")
    parser.add_argument("-g", "--group", type=argparse_types.group, required=True,
                        help="Obligatory. Group to process")
    parser.add_argument("-e", "--checkers", type=argparse_types.comma_list, required=True,
                        help="Obligatory. Comma-separated list of checkers for alive machines: one or more from <%s>" % ",".join(
                            sorted(CHECKERS.keys())))
    parser.add_argument("-p", "--replace-percent", type=float, default=DEFAULT_PERCENTS,
                        help="Optional. Maximal quota of machines to replace (in percents, %s by default)" % DEFAULT_PERCENTS)
    parser.add_argument("-c", "--comparator", type=str, default=DEFAULT_COMPARATOR,
                        help="Optional. Comparator for replace_host2 utility: %s by default" % DEFAULT_COMPARATOR)

    return parser


def normalize(options):
    pass


def main(options):
    replace_limit = int(len(options.group.getHosts()) * options.replace_percent / 100.)

    result = defaultdict(dict)

    for checker in options.checkers:
        # find unworking machines
        check_alive_machines_options = {
            'checker': checker,
            'groups': [options.group],
            'non_interactive': True,
        }
        check_alive_machines_result = check_alive_machines.jsmain(check_alive_machines_options)
        print check_alive_machines_result

        failed_hosts = check_alive_machines_result['failed_hosts']
        result[checker]['failed_hosts'] = failed_hosts

        failed_hosts_to_replace = failed_hosts[:replace_limit]
        replace_limit -= len(failed_hosts_to_replace)
        result[checker]['failed_hosts_to_replace'] = failed_hosts_to_replace

        # try to replace unworking
        replace_host2_options = {
            'comparator': options.comparator,
            'hosts': failed_hosts_to_replace,
            'dest_group': CURDB.groups.get_group(CHECKERS[checker]),
            'skip_missing': True,
            'modify': True,
            'apply': False,
        }
        replace_host2_result = replace_host2.jsmain(replace_host2_options)
        print replace_host2_result

    return None

def print_result(result):
    pass


def jsmain(d):
    options = get_parser().parse_json(d)
    normalize(options)
    return main(options)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    result = main(options)

    print_result(result)

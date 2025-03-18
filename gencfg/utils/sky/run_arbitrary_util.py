#!/skynet/python/bin/python
"""
    Sometimes we want to run util on any remote host. Usual way to do it is checkout gencfg on remote host and run util (which is uncomfortably).
    This script allow "arbitrary" util from utils/common to run on specified hosts (all necessary code are transferred with skynet engine.
    Unfortunately, we can not run scripts, which require CURDB, but this script is usefull nevertheless.
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import tempfile
import pwd
import subprocess

import api.cqueue
import library.config
from kernel.util.errors import formatException

import gencfg
from gencfg0 import _packages_path
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types

# import supported utils
import utils.common.get_storages_configuration as get_storages_configuration


SUPPORTED_UTILS = {
    'get_storages_configuration': get_storages_configuration.jsmain,
}


class TRunner(object):
    def __init__(self, util_name, args):
        self.func = SUPPORTED_UTILS[util_name]
        self.args = args
        self.osUser = pwd.getpwuid(os.getuid())[0]

    def run(self):
        fid, fname = tempfile.mkstemp()
        os.dup2(fid, sys.stdout.fileno())
        self.func(self.args)
        sys.stdout.flush()
        return open(fname).read()


def get_parser():
    parser = ArgumentParserExt(description="Run arbitrary util from utils/common")
    parser.add_argument("-u", "--util-name", type=str, required=True,
        choices = sorted(SUPPORTED_UTILS.keys()),
        help="Obligatory. Util to execute")
    parser.add_argument("-a", "--util-args", type=argparse_types.jsondict, required = True,
        help="Obligatory. Util arguments as jsondict or <k1=v1,k2=v2,...,kn=v4> format")
    parser.add_argument("-s", "--hosts", type=argparse_types.hostnames, required=True,
        help="Obligatory. Comma-separated list of hosts to run util on")
    parser.add_argument("-t", "--timeout", type=int, default=100,
        help="Optional. Cutsom timeout")
    parser.add_argument("--quiet", action="store_true", default=False,
        help="Optional. More quiet output (ignore hosts with exception, do not show host names)")

    return parser


def main(options):
    # have to update sys.path
    runner = TRunner(options.util_name, options.util_args)

    if sys.path[0] == _packages_path:
        sys.path.pop(0)
        sys.path.append(_packages_path)

    client = api.cqueue.Client('cqudp', netlibus=True, no_porto=True)
    for hostname, result, failure in client.run(options.hosts, runner).wait(options.timeout):
        if not options.quiet:
            print "===================== %s BEGIN =========================" % hostname

        if failure is not None:
            if not options.quiet:
                print "Status: FAILED"
                print formatException(failure)
        else:
            if not options.quiet:
                print "Status: SUCCESS"
            print result

        if not options.quiet:
            print "===================== %s END ===========================" % hostname

if __name__ == '__main__':
    options = get_parser().parse_cmd()

    main(options)

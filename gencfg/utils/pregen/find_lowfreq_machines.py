#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
from argparse import ArgumentParser
import re

import api.cqueue
from kernel.util.errors import formatException

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types

MINFREQS = {'E5-2660': 1300}


def parse_cmd():
    parser = ArgumentParser(description="Find machines with too low frequency")
    parser.add_argument("-g", "--groups", type=argparse_types.groups, dest="groups", required=True,
                        help="Obligatory. Comma-separated list of groups to check")
    parser.add_argument("-t", "--timeout", type=int, dest="timeout", default=10,
                        help="Optional. Skynet timeout")
    parser.add_argument("-v", "--verbose", action="store_true", dest="verbose", default=False,
                        help="Optional. Add verbose output")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


class Runner(object):
    def __init__(self, modelsdata):
        self.modelsdata = modelsdata

    def run(self):
        # detect model for freebsd and others not supported
        if os.uname()[0] != 'Linux':
            return False

        model = re.search('model name([^\n]*)', open('/proc/cpuinfo').read()).group(0).partition(': ')[2]
        if model not in self.modelsdata:
            raise Exception("Unknown model <%s>" % model)
        model = self.modelsdata[model]
        mhz = max(map(lambda x: float(x), re.findall('cpu MHz\t*: ([^\n]*)', open('/proc/cpuinfo').read())))

        if model in MINFREQS and mhz < MINFREQS[model]:
            return True
        return False


def main(options):
    allhosts = sum(map(lambda x: x.getHosts(), options.groups), [])
    modelsdata = dict(map(lambda (k, v): (v.fullname, k), CURDB.cpumodels.models.items()))

    client = api.cqueue.Client('cqudp', netlibus=True)
    badhosts = []
    for hostname, result, failure in client.run(map(lambda x: x.name, allhosts), Runner(modelsdata)).wait(
            options.timeout):
        if failure:
            if options.verbose:
                print 'HOST %s' % hostname
                print formatException(failure)
            continue
        if result is True:
            badhosts.append(hostname.split('.')[0])

    return badhosts


if __name__ == '__main__':
    options = parse_cmd()

    badhosts = main(options)
    print "Bad hosts: %s" % (','.join(badhosts))

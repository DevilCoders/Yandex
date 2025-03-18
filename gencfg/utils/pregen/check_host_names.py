#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
import socket
from collections import defaultdict
from argparse import ArgumentParser

import api.cqueue

import core.argparse.types as argparse_types

import gencfg


class GetName(object):
    def __init__(self):
        pass

    def run(self):
        return socket.gethostbyaddr(socket.gethostbyname(socket.gethostname()))[0].split('.')[0]


def parse_cmd():
    parser = ArgumentParser(description="Check if host names are correct")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=["check"],
                        help="Obligatory. Action to execute")
    parser.add_argument("-s", "--hosts", dest="hosts", type=argparse_types.hostnames, required=True,
                        help="Obligatory. List of hosts to check")
    parser.add_argument("-t", "--timeout", dest="timeout", type=int, required=False, default=20,
                        help="Optional. Execution timeout")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    namesdata = defaultdict(list)

    client = api.cqueue.Client('cqudp', netlibus=True)
    for hostname, result, failure in client.run(options.hosts, GetName()).wait(options.timeout):
        if failure is None:
            namesdata[result].append(hostname)

    print "Successfully processed %s of %s hosts" % (len(namesdata), len(options.hosts))

    print "Wrong names:"
    for k in sorted(namesdata.keys()):
        if len(namesdata[k]) > 1 or namesdata[k][0] != k:
            print "   <<%s>> -> <<%s>>" % (','.join(namesdata[k]), k)


if __name__ == '__main__':
    options = parse_cmd()
    main(options)

#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
from optparse import OptionParser

import gencfg
from core.db import CURDB


def parse_cmd():
    usage = "Usage %prog [options]"
    parser = OptionParser(usage=usage)

    parser.add_option("-g", "--groups", type="str", dest="groups", default=None,
                      help="optional. Modified groups")

    if len(sys.argv) == 1:
        sys.argv.append('-h')

    (options, args) = parser.parse_args()

    if len(args):
        raise Exception("Unparsed args: %s" % args)

    options.groups = map(lambda x: CURDB.groups.get_group(x), options.groups.split(','))

    return options


if __name__ == '__main__':
    options = parse_cmd()

    for group in options.groups:
        group.custom_instance_power.clear()
        group.mark_as_modified()

    CURDB.update(smart=True)

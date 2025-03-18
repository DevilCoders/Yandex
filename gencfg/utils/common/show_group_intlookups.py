#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from optparse import OptionParser

import gencfg
from core.db import CURDB


def parse_cmd():
    usage = "Usage %prog"
    parser = OptionParser(usage=usage)
    parser.add_option("-g", "--groups", dest="groups", help="Obligatory. Group list")
    parser.add_option("-f", "--fair", action="store_true", dest="fair", default=False,
                      help="Optional. Parse intlookups and find fair answer")
    (options, args) = parser.parse_args()

    if options.groups is None:
        raise Exception("--groups param is not set but is obligatory")
    separator = '+' if '+' in options.groups else ','
    options.groups = [CURDB.groups.get_group(group) for group in options.groups.split(separator)]

    return options, args


if __name__ == '__main__':
    (options, args) = parse_cmd()

    for group_name in sorted([group.card.name for group in options.groups]):
        print '%s: %s' % (group_name, ','.join(sorted(CURDB.groups.get_group(group_name).intlookups)))

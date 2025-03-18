#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser

import gencfg
from core.db import CURDB
from gaux.aux_colortext import red_text

NOCHECK_INTLOOKUPS = ['intlookup-msk-build-test.py', 'intlookup-sas-imgmisc.py', 'intlookup-msk-imgmisc.py',
                      'intlookup-msk-imgmisc-r1.py', 'intlookup-msk-r1.py.ondisk',
                      'intlookup-man-web-base-mmeta-experiment.py']


def parse_cmd():
    parser = ArgumentParser(description="Check for intlookups with empty groups")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=["anyempty", "allempty"],
                        help="Obligatory. Check if any of shards in intlookup empty (anyempty) or all shards empty (allempty)")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    empty_intlookups = []
    for intlookup in CURDB.intlookups.get_intlookups():
        if intlookup.file_name in NOCHECK_INTLOOKUPS:
            continue

        all_brigades = sum(map(lambda x: x.brigades, intlookup.brigade_groups), [])
        if options.action in ['allempty', 'anyempty'] and len(all_brigades) == 0:
            empty_intlookups.append(intlookup.file_name)
            continue
        if options.action == 'anyempty':
            if 0 in map(lambda x: len(x.brigades), intlookup.brigade_groups):
                empty_intlookups.append(intlookup.file_name)
            continue

    if len(empty_intlookups):
        if options.action == 'allempty':
            prefix = "Empty intlookups"
        elif options.action == 'anyempty':
            prefix = "Partially empty intlookups"

        print red_text("%s (%d total): %s" % (prefix, len(empty_intlookups), ' '.join(empty_intlookups)))

        return 1

    return 0


if __name__ == '__main__':
    options = parse_cmd()
    retcode = main(options)
    sys.exit(retcode)

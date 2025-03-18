#!/skynet/python/bin/python
"""Script to manipulate ctypes"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
from core.ctypes import CType


class EActions(object):
    ADD = 'add'  # add new ctype
    ALL = [ADD]


def get_parser():
    parser = ArgumentParserExt(description='Manipulate with ctypes')
    parser.add_argument('-a', '--action', type=str, required=True,
                        choices=EActions.ALL,
                        help='Obligatory. Action to execute')
    parser.add_argument('-c', '--ctype', type=str, default=None,
                        help='Optional. Itype name (for actions {})'.format(EActions.ADD))
    parser.add_argument('-d', '--descr', type=str, default=None,
                        help='Optional. Itype description (for actions {})'.format(EActions.ADD))

    return parser


def normalize(options):
    if options.action == EActions.ADD:
        if (options.ctype is None) or (options.descr is None):
            raise Exception('You must specify <--ctype> and <--descr> option in action <{}>'.format(options.action))


def main(options):
    if options.action == EActions.ADD:
        CURDB.ctypes.add_ctype(CType(CURDB.ctypes, dict(name=options.ctype, descr=options.descr)))
        CURDB.ctypes.update(smart=True)
    else:
        raise Exception('Unknown action <{}>'.format(options.action))


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)

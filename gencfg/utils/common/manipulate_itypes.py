#!/skynet/python/bin/python
"""Script to manipulate itypes"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
from core.itypes import IType


class EActions(object):
    ADD = 'add'  # add new itype
    ALL = [ADD]


def get_parser():
    parser = ArgumentParserExt(description='Manipulate with itypes')
    parser.add_argument('-a', '--action', type=str, required=True,
                        choices=EActions.ALL,
                        help='Obligatory. Action to execute')
    parser.add_argument('-i', '--itype', type=str, default=None,
                        help='Optional. Itype name (for actions {})'.format(EActions.ADD))
    parser.add_argument('-d', '--descr', type=str, default=None,
                        help='Optional. Itype description (for actions {})'.format(EActions.ADD))

    return parser


def normalize(options):
    if options.action == EActions.ADD:
        if (options.itype is None) or (options.descr is None):
            raise Exception('You must specify <--itype> and <--descr> option in action <{}>'.format(options.action))


def main(options):
    if options.action == EActions.ADD:
        CURDB.itypes.add_itype(IType(CURDB.itypes, dict(name=options.itype, descr=options.descr, config_type=None)))
        CURDB.itypes.update(smart=True)
    else:
        raise Exception('Unknown action <{}>'.format(options.action))


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)

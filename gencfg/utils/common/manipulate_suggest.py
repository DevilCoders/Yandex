#!/skynet/python/bin/python
"""Script for various manipulation with suggest (RX-531)"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import math

import gencfg
from core.db import CURDB
import core.argparse.parser
import core.argparse.types as argparse_types
from core.audit.cpu import suggest_for_group
from core.card.types import Date


class EActions(object):
    SUGGEST_CPU = 'suggest_cpu'  # suggest cpu for groups (RX-531)
    ALL = [SUGGEST_CPU]


def get_parser():
    parser = core.argparse.parser.ArgumentParserExt(description='Manipulate suggest in various ways')
    parser.add_argument('-a', '--action', type=str, required=True,
                        choices=EActions.ALL,
                        help='Obligatory. Action to execute')
    parser.add_argument("-g", "--groups", type=argparse_types.groups, default='ALL',
                        help="Obligatory. Groups to perform action on")
    parser.add_argument('-y', '--apply', action='store_true', default=False,
                        help='Optional. Apply changes')

    return parser


def main_suggest_cpu(options):
    for group in options.groups:
        suggestion = suggest_for_group(group)

        if suggestion.power is None:
            print '    Can not suggestion anything: {}'.format(suggestion.msg)
        else:
            print '    {} ({} total)'.format(suggestion.msg, suggestion.power * len(group.get_kinda_busy_instances()))

        if options.apply:
            group.card.audit.cpu.suggest.msg = suggestion.msg
            if suggestion.power is not None:
                group.card.audit.cpu.suggest.cpu = float(math.ceil(suggestion.power))
            else:
                group.card.audit.cpu.suggest.cpu = suggestion.power
            group.card.audit.cpu.suggest.at = Date.create_from_duration(0)
            group.mark_as_modified()

    if options.apply:
        CURDB.update(smart=True)


def main(options):
    if options.action == EActions.SUGGEST_CPU:
        main_suggest_cpu(options)
    else:
        raise Exception('Unknown action <{}>'.format(options.action))


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    main(options)

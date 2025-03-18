#!/skynet/python/bin/python
"""
    In this utility we copy part of card from one group to anothers
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import copy

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types
from gaux.aux_colortext import red_text

def get_parser():
    parser = ArgumentParserExt(description="Copy part of card from one group to anothers")
    parser.add_argument("-s", "--src-group", type=core.argparse.types.group, required=True,
        help="Obligatory. Source group to copy card from")
    parser.add_argument("-d", "--dst-groups", type=core.argparse.types.groups, required=True,
        help="Obligatory. Comma-separated list of groups to replace card")
    parser.add_argument("-p", "--paths", type=core.argparse.types.comma_list, required=True,
        help="Obligatory. Comma-separated list of paths to copy")
    parser.add_argument("-y", "--apply", action="store_true", default=False,
        help="Optional. Apply changes")
    parser.add_argument("-v", "--verbose", action="count", default=0,
        help="Optional. Verbosity level (multiple optoin increases verbosity)")

    return parser

def normalize(options):
    # check if paths exist
    options.paths = map(lambda x: x.split('.'), options.paths)
    for path in options.paths:
        options.src_group.card.resolve_card_path(path)

def main(options):
    for path in options.paths:
        src_node = options.src_group.card.resolve_card_path(path)

        for group in options.dst_groups:
            dst_node = group.card.resolve_card_path(path)
            if isinstance(dst_node, list):
                del dst_node[:]
                dst_node.extend(copy.deepcopy(src_node))
            else:
                dst_node.replace_self(copy.deepcopy(src_node))
            group.mark_as_modified()

    if options.apply:
        CURDB.groups.update(smart=True)
    else:
        print red_text("Not updated!!! Add option -y to update.")

if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)


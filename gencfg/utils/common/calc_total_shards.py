#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.argparse.parser import ArgumentParserExt
from core.db import CURDB


def get_parser():
    parser = ArgumentParserExt(description="Show total number of shards in shards experession")
    parser.add_argument("-s", "--shards-expression", dest="shards_expression", type=str, required=True,
                        help="Obligatory. Shard exprssion, like RRGTier0+PlatinumTier0")
    parser.add_argument("--brief", action="store_true", default=False,
                        help="Optional. Print result briefly")

    return parser


def main(options):
    return CURDB.tiers.primus_int_count(options.shards_expression)


def jsmain(d):
    options = get_parser().parse_json(d)
    return main(options)


def print_result(result, options):
    if options.brief:
        print result[0]
    else:
        print result


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    result = main(options)

    print_result(result, options)

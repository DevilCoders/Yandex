#!/skynet/python/bin/python
"""
    Create big json with mapping for all gencfg groups to instances:
    {
        group1: {
            instances: [
                {
                    host: host_name,
                    port: port_name,
                },
                ...
            ],
        },
        group2: {
            ...
        },
        ...
    }
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import simplejson

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types

def get_parser():
    parser = ArgumentParserExt(description='Generate big mapping ')
    parser.add_argument('-o', '--output-file', type=str, required=True,
        help='Obligatory. File to save mapping to')
    parser.add_argument("--db", type=argparse_types.gencfg_db, default=CURDB,
                        help="Optional. Path to db")

    return parser


def main(options):
    result = {}
    for group in options.db.groups.get_groups():
        group_instances = [dict(host=x.host.name, port=x.port) for x in group.get_kinda_busy_instances()]
        result[group.card.name] = dict(instances=group_instances)

    with open(options.output_file, 'w') as f:
        f.write(simplejson.dumps(result))


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    main(options)

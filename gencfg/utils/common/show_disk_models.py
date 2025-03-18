#!/skynet/python/bin/python
"""
    Show available disks, used in search
"""


import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.argparse.parser import ArgumentParserExt

def get_parser():
    parser = ArgumentParserExt("Show information on used in search disk models")
    parser.add_argument("-i", "--show-fields", type=argparse_types.comma_list, required=True,
        help="Obligatory. Show specified fields for disk model card")
    parser.add_argument("-f", "--filter", type=argparse_type.pythonlambda, default=lambda x: True,
        help="Optional. Filter on disks to show")

    return parser

def normalize(options):
    # check for invalid fields
    for field_name in options.show_fields:
        if field_name in SHOW_FIELDS:
            continue
        try:
            options.db.groups.get_scheme()._scheme.resolve_scheme_path(field_name.split('.'))
        except core.card.node.ResolveCardPathError:
            raise Exception("Field <%s> not found in group card" % field_name)


def main(options):
    models = CURDB.cpumodels.get_models()
    models = filter(options.filter, models)

if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)

#!/skynet/python/bin/python
"""Change int power to be proportional to int cpu load.

By default, all int instances have same cpu power. However basesearch groups power can vary a lot. Thus we do not need much power
for ints, servicing zero-power groups"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types
from gaux.aux_colortext import red_text


def get_parser():
    parser = ArgumentParserExt(description='Change ints power')
    parser.add_argument('-i', '--intlookup', type=core.argparse.types.intlookup, required=True,
                        help='Obligatory. Intlookup to process')
    parser.add_argument('--coeff', type=float, required=True,
                        help='Obligatory. Ratio of total ints power to basesearhcer power for TIntl2Group')
    parser.add_argument('--zero-weight-power', type=int, default=20,
                        help='Optional. Weights of ints with zero load')
    parser.add_argument('-y', '--apply', action='store_true', default=False,
                        help='Optional. Apply changes')

    return parser


def main(options):
    # process intlookup
    for int_group in options.intlookup.get_int_groups():
        load_power = int_group.power * options.coeff
        load_power_per_int = round(load_power / float(len(int_group.intsearchers)), 1)
        for instance in int_group.intsearchers:
            instance.power = load_power_per_int + options.zero_weight_power
    options.intlookup.mark_as_modified()

    # process groups
    groups = {x.type for x in options.intlookup.get_int_instances()}
    groups = [CURDB.groups.get_group(x) for x in groups]
    for group in groups:
        group.custom_instance_power_used = True
        group.mark_as_modified()

    # apply changes
    if options.apply:
        CURDB.update(smart=True)
    else:
        print red_text('Not applied. Add <--apply> option to apply')

if __name__ == '__main__':
    options = get_parser().parse_cmd()

    main(options)

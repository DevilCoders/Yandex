#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser

import gencfg
import core.argparse.types as argparse_types
from core.db import CURDB


def parse_cmd():
    parser = ArgumentParser(description="Check number of snippets replicas (compare with sas config)")
    parser.add_argument("-c", "--config", type=argparse_types.sasconfig, required=True,
                        help="Obligatory. Sas config")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def _is_good_snippet_brigade(brigade):
    return len(filter(lambda x: x.host.ssd == 0, brigade.get_all_basesearchers())) == 0


def check_replicas(intlookup, needed_ssd_replicas):
    failed = False
    for brigade_group in intlookup.get_multishards():
        ssd_replicas = len(filter(_is_good_snippet_brigade, brigade_group.brigades))
        if ssd_replicas < needed_ssd_replicas:
            baseserch = brigade_group.brigades[0].basesearchers[0][0]
            print "TIntGroup group %s has only %s ssd replicas" % (baseserch.full_name(), ssd_replicas)
            failed = True

    return failed


def main(options):
    data = map(lambda x: (CURDB.intlookups.get_intlookup(x.intlookup), x.ssdreplicas), options.config)

    failed = False
    for intlookup, needed_ssd_replicas in data:
        failed |= check_replicas(intlookup, needed_ssd_replicas)

    return failed


if __name__ == '__main__':
    options = parse_cmd()

    failed = main(options)

    sys.exit(failed)

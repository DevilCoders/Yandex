#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from optparse import OptionParser

import gencfg
from core.db import CURDB


def parse_cmd():
    usage = "Usage %prog [options]"
    parser = OptionParser(usage=usage)
    parser.add_option("-i", "--intlookup-file", type="str", dest="intlookup_file",
                      help="specify intlookup file")
    parser.add_option("-n", "--number-of-replicas", type="int", dest="number_of_replicas",
                      help="specify number of replicas")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    (options, args) = parser.parse_args()

    return options


if __name__ == '__main__':
    options = parse_cmd()

    intlookup = CURDB.intlookups.get_intlookup(os.path.basename(options.intlookup_file))
    intlookup.mark_as_modified()

    N = options.number_of_replicas
    for multishard in intlookup.get_multishards():
        if len(multishard.brigades) < N:
            raise Exception("Not enough replicas")
        multishard.brigades = multishard.brigades[:N]

    CURDB.intlookups.update(smart=True)

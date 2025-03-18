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
    parser.add_option("-m", "--output-mode", type="choice", dest="output_mode", default="instances",
                      choices=["instances", "hosts", "+hosts", "hosts_by_dc"],
                      help="select for what: \"instances\", \"hosts\", \"+hosts\", \"hosts_by_dc\"")
    parser.add_option("-t", "--instance-type", type="choice", dest="instance_type",
                      choices=["int", "base"],
                      help="select instance type: \"int\" or \"base\"")
    parser.add_option("-p", "--port", type="int",
                      help="specify port")

    if len(sys.argv) == 1:
        sys.argv.append('-h')

    (options, args) = parser.parse_args()

    return options


if __name__ == '__main__':
    options = parse_cmd()
    TYPE = options.instance_type
    WHAT = options.output_mode
    PORT = options.port

    intlookup = CURDB.intlookups.get_intlookup(os.path.basename(options.intlookup_file))

    # generated instances list, filter by type
    if TYPE == "int":
        instances = intlookup.get_used_int_instances()
    elif TYPE == "base":
        instances = intlookup.get_used_base_instances()
    else:
        instances = intlookup.get_used_base_instances() + intlookup.get_used_int_instances()

    # filter by PORT
    if PORT is not None:
        instances = filter(lambda x: x.port == PORT, instances)

    if WHAT == "instances":
        print ' '.join(sorted(map(lambda x: "%s:%s" % (x.host.name, x.port), instances)))
    elif WHAT == "hosts":
        print ','.join(map(lambda x: x.host.name, instances))
    elif WHAT == "+hosts":
        print ' '.join(map(lambda x: "+%s" % x.host.name, instances))
    elif WHAT == "hosts_by_dc":
        for dc in set(map(lambda x: x.dc, instances)):
            print "%s: %s" % (dc, ' '.join(set(map(lambda x: x.host.name, filter(lambda x: x.dc == dc, instances)))))

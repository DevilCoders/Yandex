#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB
from core.hosts import Host
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
from gaux.aux_colortext import red_text


def get_parser():
    avail_fields = map(lambda x: x.name, filter(lambda x: x.primary == True, Host.SCHEME))

    parser = ArgumentParserExt("Script to replace unworking machines by working ones")
    parser.add_argument("-s", "--hosts", type=argparse_types.hosts, required=True,
                        help="Obligatory. List of hosts to process")
    parser.add_argument("-f", "--field-name", type=str, required=True,
                        choices=avail_fields,
                        help="Obligatory. Field name to update")
    parser.add_argument("-e", "--field-value", type=str, required=True,
                        help="Obligatory. Field value to update")
    parser.add_argument("-y", "--apply", dest="apply", action="store_true", default=False,
                        help="Optional. Apply changes (otherwise db will not be changed)")
    parser.add_argument("-v", "--verbose", action="count", dest="verbose_level", default=0,
                        help="Optional. Verbose mode. Multiple -v options increase the verbosity.  The maximum is 1.")

    return parser


def normalize(options):
    scheme_field = filter(lambda x: x.name == options.field_name, Host.SCHEME)[0]
    options.field_value = scheme_field.type(options.field_value)


def main(options):
    for host in options.hosts:
        if options.verbose_level > 0:
            print "Host %s change field <%s>: %s -> %s" % (
            host.name, options.field_name, getattr(host, options.field_name), options.field_value)
        setattr(host, options.field_name, options.field_value)

    if options.apply:
        CURDB.hosts.update()
    else:
        print red_text("Not updated!!! Add option -y to update.")


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    main(options)

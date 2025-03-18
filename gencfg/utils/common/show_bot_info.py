#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import urllib2
from argparse import ArgumentParser
import json

import gencfg
import core.argparse.types as argparse_types
import gaux.aux_utils


INFOBYINVNUM_URL = "http://bot.yandex-team.ru/api/osinfo.php?inv=%s&output=XXCSI_FQDN&format=json"

SHOWFIEDLS_URL = ""
SHOWFIELDS_MAPPING = {
    'switch': 'switch_short',
    'queue': 'short_line',
    'dc': 'short_dc',
    'vlan': 'primary_vlan',
    'rack': 'rack',
    'invnum': 'inventory_number',
}


def parse_cmd():
    ACTIONS = ['infobyinvnum', 'showfield']

    parser = ArgumentParser(description="Add extra replicas to intlookup (from free instances)")

    parser.add_argument("-a", "--action", type=str, default='infobyinvnum',
                        choices=ACTIONS,
                        help="Obligatory. Kind of data to show: %s" % ",".join(ACTIONS))
    parser.add_argument("-i", "--invnum", type=str, default=None,
                        help="Optional. Host inventory number")
    parser.add_argument("-s", "--host", type=argparse_types.host, default=None,
                        help="Optional. Host name")
    parser.add_argument("-f", "--fields", type=argparse_types.comma_list, default=[],
                        help="Optional. Fields to show: %s (for action showfield)" % ",".join(
                            sorted(SHOWFIELDS_MAPPING.keys())))
    parser.add_argument("-v", "--verbose-level", action="count", default=0,
                        help="Optional. Verbose mode. Multiple -v options increase the verbosity. The maximum is 1.")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    if not ((options.invnum is None) ^ (options.host is None)):
        raise Exception("You must spcify only one of invnum,host options")

    if options.action == "showfield" and options.invnum is not None:
        raise Exception("Option <--action showfield> is incompatible with option <--invnum>")

    if options.fields == ["all"]:
        options.fields = SHOWFIELDS_MAPPING.keys()

    return options


def show(options, result):
    if options.action == "infobyinvnum":
        invnum, data = result
        if options.host is None:
            print "Invnum %s" % invnum
        else:
            print "Invnum %s (host %s)" % (invnum, options.host.name)
        print data
    elif options.action == "showfield":
        host, fields_data = result
        fields_str = " ".join(map(lambda (field_name, gencfg_result, bot_result): "(%s: gencfg - <%s>, bot - <%s>)" % (
        field_name, gencfg_result, bot_result), fields_data))
        print "Host %s: %s" % (host.name, fields_str)


def main(options):
    if options.action == "infobyinvnum":
        if options.invnum is None:
            options.invnum = options.host.invnum

            if options.invnum in ["", "unknown"]:
                raise Exception("Host %s has incorrect invnum <%s>" % (options.host.name, options.invnum))

        url = INFOBYINVNUM_URL % options.invnum

        if options.verbose_level > 0:
            print "Fetch URL: %s" % url

        request = urllib2.Request(url)

        data = urllib2.urlopen(request, timeout=10).read().strip()
        data = json.loads(data)
        data = json.dumps(data, indent=4)
        data = gaux.aux_utils.indent(data)

        return options.invnum, data
    elif options.action == "showfield":
        result = []
        for field_name in options.fields:
            url = SHOWFIEDLS_URL % (options.host.name, SHOWFIELDS_MAPPING[field_name])

            if options.verbose_level > 0:
                print "Fetch URL: %s" % url

            bot_field = urllib2.urlopen(url, timeout=10).read().strip()
            gencfg_field = getattr(options.host, field_name)
            result.append((field_name, gencfg_field, bot_field))
        return options.host, result
    else:
        raise Exception("Unknown action %s" % options.action)


if __name__ == '__main__':
    options = parse_cmd()
    result = main(options)
    show(options, result)

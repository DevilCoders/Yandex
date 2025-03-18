#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

import re
from argparse import ArgumentParser
import urllib2

import gencfg
from utils.sky import blinov
import core.argparse.types as argparse_types


# URL = "https://git.qe-infra.yandex-team.ru/projects/SEARCH_INFRA/repos/yasm/browse/CONF/agent.base.conf?&raw"

def parse_cmd():
    parser = ArgumentParser(description="Script to check if all golovan filters leads to non-zero host intersection")
    parser.add_argument("-u", "--url", type=str, required=True,
                        help="Obligatory. Stash url to directory with configs")
    parser.add_argument("-s", "--searcherlookup", type=argparse_types.stripped_searcherlookup, required=True,
                        help="Obligatory. Path to searcherlookup (otherwise searcherlookup will be generated")
    parser.add_argument("-v", "--verbose", action="store_true", default=False,
                        help="Optional. Add verbose output")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def process_itype(golovan_data, searcherlookup, itype, verbose):
    m = re.search("(\[tags\]|\[newtags\])([\s\S]*?)\[", golovan_data)
    if not m:
        raise Exception("Can not parse file")
    data = m.group(2)
    data = data.split('\n')
    data = map(lambda x: x.split(' ')[0], filter(lambda x: x.find('=') > 0, data))
    data = filter(lambda x: len(x.split('_')) == 4, data)

    for elem in data:
        ctype, prj, geo, tier = elem.split('_')
        if tier == 'self':
            blinov_string = "itag=a_ctype_%s AND itag=a_prj_%s AND itag=a_geo_%s AND itag=a_itype_%s" % (ctype, prj, geo, itype)
        else:
            blinov_string = "itag=a_ctype_%s AND itag=a_prj_%s AND itag=a_geo_%s AND itag=a_tier_%s AND itag=a_itype_%s" % (ctype, prj, geo, tier, itype)

        blinov_options = {
            "filter": blinov_string,
            "quiet": True,
            "searcherlookup": searcherlookup,
            "compare_filter": None,
        }
        r = blinov.main(type("DummyBlinovOptons", (), blinov_options)())

        if verbose:
            print "    Elem %s has %d instances" % (elem, len(r[0]))
        else:
            if len(r[0]) == 0:
                print "    %s" % elem


def main(options):
    data = urllib2.urlopen(options.url, timeout=10).read()
    itypes = re.findall('agent\.([^.]*?)\.conf', data)
    itypes = filter(lambda x: x != 'common', itypes)
    print "Will process itypes: %s" % (' '.join(itypes))

    for itype in itypes:
        print "Processing %s:" % itype

        tagurl = "%s/agent.%s.conf?&raw" % (options.url, itype)
        golovan_data = urllib2.urlopen(tagurl, timeout=10).read()

        try:
            process_itype(golovan_data, options.searcherlookup, itype, options.verbose)
        except:
            print "OOPS"


if __name__ == '__main__':
    options = parse_cmd()
    main(options)

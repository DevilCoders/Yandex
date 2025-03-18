#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
from argparse import ArgumentParser

from api.cms import Yr

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types
from core.instances import FakeInstance
from core.hosts import FakeHost


class OrProcessor(object):
    def __init__(self, s):
        self.tags = []
        self.yr_tags = []
        self.filters = []

        for elem in s.split(' '):
            if elem.startswith('I@'):
                elem = elem.replace('I@', 'itag=')
            if elem.startswith('H@'):
                elem = elem.replace('H@', 'host=')
            if elem.startswith('+') or elem.startswith('-'):
                self.yr_tags.append((elem.split(':')[0], elem.split(':')[1]))
            elif elem.startswith('host='):
                self.filters.append(elem.split('=')[1])
            else:
                self.tags.append(elem)

    def process(self, data, options):
        result = []

        for tag in self.tags:
            if tag not in data.tags:
                msg = "WARNING: no tag <<%s>> in searcherlookup !!!!!" % tag
                if options.werror:
                    raise Exception(msg)
                else:
                    print msg
        result.extend(sum(map(lambda x: data.tags.get(x, []), self.tags), []))

        if len(self.yr_tags):
            port = self.yr_tags[0][1]
            yr_raw = []
            for yr_tag, yr_port in self.yr_tags:
                if yr_port != port:
                    raise Exception("Invalid port %s for group %s" % (yr_port, yr_tag))
                yr_raw.append(yr_tag)

            result.extend(map(lambda x: FakeInstance(FakeHost(x.split('.')[0]), port), Yr.listServersRaw(*yr_raw)))

        for flt in self.filters:
            result.extend(filter(lambda x: x.host.name.startswith(flt), data.instances))

        return set(result)


class AndProcessor(object):
    def __init__(self, s):
        self.orProcessors = map(lambda x: OrProcessor(x), s.replace(' . ', ' AND ').split(' AND '))

    def process(self, data, options):
        return set.intersection(*map(lambda x: x.process(data, options), self.orProcessors))


class MinusProcessor(object):
    def __init__(self, s):
        ress = []
        for minus_elem in s.split(' MINUS '):
            plus_elems = minus_elem.split(' PLUS ')
            ress.append(plus_elems[0])
            ress[0] = ' '.join([ress[0]] + plus_elems[1:])

        self.andProcessors = map(lambda x: AndProcessor(x), ress)

    def process(self, data, options):
        if len(self.andProcessors) == 1:
            return self.andProcessors[0].process(data, options)
        else:
            return self.andProcessors[0].process(data, options) - set.union(
                *map(lambda x: x.process(data, options), self.andProcessors[1:]))


def parse_filter(expr):
    return MinusProcessor(expr)


def correct_searcherlookup(searcherlookup):
    searcherlookup.tags = {}
    for itag in searcherlookup.itags_auto:
        searcherlookup.tags['itag=%s' % itag] = searcherlookup.itags_auto[itag]
    for stag in searcherlookup.stags:
        searcherlookup.tags['stag=%s' % stag] = searcherlookup.stags[stag]


def parse_cmd():
    parser = ArgumentParser(description="Limited blinov calculator", prog=sys.argv[0], usage="""
        %(prog)s -f \"itag=MSK_WEB_BASE AND itag=a_tier_PlatinumTier0\" -c \"I@MSK_WEB_BASE . I@a_tier_PlatinumTier0\"
        %(prog)s -f \"itag=MSK_RESERVED\" -s w-generated/searcherlookup.conf
        %(prog)s -f +ABALANCER:8080
        %(prog)s -f H@ws2-200""")

    parser.add_argument("-s", "--searcherlookup", dest="searcherlookup", type=argparse_types.stripped_searcherlookup,
                        help="Optional. Path to searcherlookup (otherwise searcherlookup will be generated")
    parser.add_argument("-f", "--filter", dest="filter", type=str, required=True,
                        help="Obligatory. HEAD.conf filter")
    parser.add_argument("-c", "--compare-filter", dest="compare_filter", type=str, default=None,
                        help="Optional. HEAD.conf compare filter")
    parser.add_argument("-q", "--quiet", dest="quiet", action="store_true", default=False,
                        help="Optional. Quiet output")
    parser.add_argument("--werror", action="store_true", default=False,
                        help="Optional. Treat warnings as errors")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options, db=CURDB):
    # make tags
    if options.searcherlookup is None:
        options.searcherlookup = db.build_searcherlookup()

    correct_searcherlookup(options.searcherlookup)

    filtered1 = MinusProcessor(options.filter)
    result1 = filtered1.process(options.searcherlookup, options)
    result2 = None

    if options.compare_filter is not None:
        filtered2 = MinusProcessor(options.compare_filter)
        result2 = filtered2.process(options.searcherlookup, options)

    return result1, result2


if __name__ == '__main__':
    options = parse_cmd()

    result1, result2 = main(options)

    if options.compare_filter is None:
        dt = ' '.join(map(lambda x: '%s:%s' % (x.host.name, x.port), result1))
        if options.quiet:
            print "%s" % dt
        else:
            print "filter instances (%d total): %s" % (len(result1), dt)
    else:
        dt1 = set(map(lambda x: '%s:%s' % (x.host.name, x.port), result1))
        dt2 = set(map(lambda x: '%s:%s' % (x.host.name, x.port), result2))

        print "filter instances (%d total, %d not found in compare_filter): %s" % (
        len(dt1), len(list(dt1 - dt2)), ' '.join(list(dt1 - dt2)[:20]))
        print "compare_filter instances (%d total, %d not found in filter): %s" % (
        len(dt2), len(list(dt2 - dt1)), ' '.join(list(dt2 - dt1)[:20]))

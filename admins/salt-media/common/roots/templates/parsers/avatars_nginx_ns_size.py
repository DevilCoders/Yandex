#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import sys
from collections import defaultdict
import os
import subprocess


def parseArgs(argv):
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option("--cache", help="[ %default ]", default=False, action='store_true')
    parser.add_option("--cache_file", help="[ %default ]", default='/var/cache/libmastermind/mds.cache')
    parser.add_option("--avatars", help="[ %default ]", default=False, action='store_true')
    parser.add_option("--dashing", help="[ %default ]", default=False, action='store_true')
    parser.add_option("--grng", help="[ %default ]", default=False, action='store_true')
    (options, arguments) = parser.parse_args(argv)
    if (len(arguments) > 1):
        raise Exception("Tool takes no arguments")
    return options


def tskv_parse(line):
    result = {}
    for x in line[5:].split('\t'):
        spl = x.split('=', 1)
        try:
            key = spl[0]
            value = spl[1]
        except IndexError:
            continue

        result[key] = value

    return result


def get_ns_cache(cache_option, cache_file):
    # mds кэш. Если options.cache = False, то cache = [].
    # Иначе в cache массив неймспейсов.
    ns_list = []
    if cache_option:
        if os.path.exists('/usr/bin/mmc-list-namespaces'):
            try:
                cmd = '/usr/bin/mmc-list-namespaces --file {0}'.format(cache_file)
                p = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE)
                ns_list = p.communicate()[0].split('\n')[:-1]
            except:
                ns_list = []
    return ns_list


def parse_request(line, cache, avatars=False):
    try:
        ls = line.split('/')
        # ['', 'get-entity_search', '30719', '38847489', 'S70x70Fit']

        result = {}
        result['request_type'] = ls[1].split('-', 1)[0]
        ns = ls[1].split('-', 1)[1]
        result['ns'] = ns
        if avatars:
            ns = "avatars-%s" % ns

        if len(cache) == 0 or ns in cache:
            if result['request_type'] == 'get' or result['request_type'] == 'delete':
                result['size'] = ls[4].split('?')[0].split('&')[0]
            else:
                return None
            return result
        else:
            return None
    except:
        return None


def modify_result(result, tskv, request_parse):
    code = int(tskv['status'])
    if 200 <= code < 300:
        status = '2xx'
    elif 300 <= code < 400:
        status = '3xx'
    elif 400 <= code < 500:
        status = '4xx'
    elif 500 <= code:
        status = '5xx'

    if not len(result[request_parse['ns']]):
        result[request_parse['ns']] = defaultdict(dict)

    if not len(result[request_parse['ns']][request_parse['size']]):
        result[request_parse['ns']][request_parse['size']] = defaultdict(int)

    result[request_parse['ns']][request_parse['size']][status] += 1
    result[request_parse['ns']][request_parse['size']]['all'] += 1

    return result


def print_dashing(result):
    res = ''
    for ns, ns_result in result.iteritems():
        res += "%s/" % ns
        for size, size_result in ns_result.iteritems():
            res += "%s." % size
            for code, code_result in size_result.iteritems():
                res += "%s:%d;" % (code, code_result)
            res = res[:-1] + ','
        res = res[:-1] + '\t'
    print res


def print_graphite(result):
    for ns, ns_result in result.iteritems():
        for size, size_result in ns_result.iteritems():
            for code, code_result in size_result.iteritems():
                print "%s.%s.%s %d" % (ns, size, code, code_result)


def main(argv):
    options = parseArgs(argv)

    cache = get_ns_cache(options.cache, options.cache_file)

    result = defaultdict(dict)
    for line in sys.stdin:
        line = line.split('\n')[0]

        tskv = tskv_parse(line)
        request_parse = parse_request(tskv['request'], cache, options.avatars)

        if request_parse is None:
            continue

        result = modify_result(result, tskv, request_parse)

    if options.dashing:
        print_dashing(result)
    if options.grng:
        print_graphite(result)


if (__name__ == "__main__"):
    sys.exit(main(sys.argv))

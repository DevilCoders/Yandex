#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import os
import sys
import msgpack
import re
from collections import defaultdict


def parseArgs(argv):
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option(
        "--cache", help="[ %default ]", default=False, action='store_true')
    parser.add_option(
        "--cache_file", help="[ %default ]", default='/var/cache/libmastermind/mds.cache')
    parser.add_option(
        "--mds", help="[ %default ]", default=False, action='store_true')
    parser.add_option(
        "--output", help="[ Media combaine (grng); Dashing (dashing); Default - %default ]", default='grng')
    parser.add_option(
        "--print_error_parse", help="[ %default ]", default=False, action='store_true')
    (options, arguments) = parser.parse_args(argv)
    if (len(arguments) > 1):
        raise Exception("Tool takes no arguments")
    return options


def tskv_parse(logs, avatars=False, mds=False):
    if logs[:5] == "tskv\t":
        fields = ['status', 'request', 'vhost', 'upstream_cache_status', 'upstream_status', 'bytes_sent',
                  'upstream_response_time', 'request_time', 'scheme', 'size', 'processing_time']
        row = dict(token.split("=", 1) for token in logs[5:].strip().split("\t") if token.split("=", 1)[0] in fields)
        if not ignore_req(row['request']):
            return row
        else:
            return None
    else:
        return None


def ignore_req(request):
    ignore = [
        '/unistorage_traffic',
        '/timetail',
        '/dv_preview',
        '/ping',
        '/',
        '/ping-alive',
        '/favicon.ico',
        '-',
        '/rules.abe',
        '/robots.txt',
        '/crossdomain.xml',
        '/cocaine-tool-info',
        '/ping_pattern',
        '/traffic',
    ]
    for x in ignore:
        if request == x:
            return True
    if (request.startswith('timetail?') or
            request.startswith('/exec_pattern') or
            request.startswith('/timetail?') or
            request.startswith('/cache?')):
        return True
    return False


def get_ns_cache(cache_option, cache_file):
    # mds кэш. Если options.cache = False, то cache = [].
    # Иначе в cache массив неймспейсов.
    ns_list = []
    if cache_option:
        if os.path.exists('/usr/bin/mmc-list-namespaces'):
            try:
                cache_file = glob.glob("{}*".format(cache_file))[0]
                cmd = '/usr/bin/mmc-list-namespaces --file {0}'.format(cache_file)
                p = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE)
                ns_list = p.communicate()[0].split('\n')[:-1]
            except:
                ns_list = []
    return ns_list


def parse_request(line, vhost, cache, mds=False):
    # Парсинг запроса. В функцию передается запрос, вида
    # /get-yapic/15298/44424292-2799728/islands-middle?rnd=1441819969643
    # Возвращает словарь.
    # Если options.avatars_sizes, то в словарь добавляются размеры (актуально для аватарницы)
    # {'request_type': 'get', 'ns': 'ynews', 'size': '160x80'}
    # Иначе {'request_type': 'get', 'ns': 'ynews'}
    try:
        # Fix
        # //get-yabs_performance/16048/2a0000014ffcddd52f0ad6c7defe3bd1d412/big
        line = re.sub('/{1,}', '/', line)

        if mds:
            return mds_request(line, vhost, cache, mds)
        else:
            return None
    except:
        return None


def mds_request(line, vhost, cache, mds=False):
    result = {}
    ls = line.split('/')
    # /get/weather-embed/forecasts-by-geo/24877.xml?embed=1
    # /upload/weather-embed/forecasts/8221.xml?timestamp=1444995769&ioflags=1&embed=1

    # Регулярка  для ocraas
    ocraas_reg = re.compile("ocr")

    # Регулярка  для ocraas
    imageparser_reg = re.compile("process")

    # Регулярка  для mds-like-ns
    mdslike_reg = re.compile("get-")

    if ls[1] in ['get-mp3']:
        result['request_type'] = 'get'
        result['ns'] = 'music'
    elif ls[1] in ['get']:
        result['request_type'] = 'get'
        result['ns'] = 'tikaite'
    elif ocraas_reg.match(ls[1]):
        result['request_type'] = 'get'
        result['ns'] = 'ocraas'
    elif imageparser_reg.match(ls[1]):
        result['request_type'] = 'get'
        result['ns'] = 'imageparser'
    elif ls[1] in ['rdisk']:
        result['request_type'] = 'get'
        result['ns'] = 'disk'
    else:
        rt = ''
        if line in ['/traffic', '/ping-alive']:
            result['request_type'] = ls[1]
            result['ns'] = 'no_ns'
        else:
            rt = 'get'
            result['request_type'] = rt
            if mdslike_reg.match(ls[1]):
                ns = ls[1].split('-', 1)[1]
            else:
                ns = ls[1]
            result['ns'] = ns
            if len(cache) != 0 and ns not in cache:
                return None
    return result


def modify_result(result, sizes_result, line, request_parse, avatars=False, avatars_sizes=False, mds=False):
    status = line.get('status')
    size = line.get('size')
    request = line.get('request')
    upstream_cache_status = line.get('upstream_cache_status')
    # vhost = line.get('vhost')
    # upstream_status = line.get('upstream_status')
    # scheme = line.get('scheme')
    bytes_sent = line.get('bytes_sent')
    upstream_response_time = line.get('upstream_response_time')
    request_time = line.get('request_time')
    processing_time = line.get('processing_time')
    if not size:
        size = bytes_sent
    if not processing_time:
        processing_time = request_time

    if avatars_sizes and request_parse['request_type'] in ['get']:
        # Тут статусы по размерам аватарницы. Нужно для таблички на дешинге.
        code = int(status)
        if 200 <= code < 300:
            status_size = '2xx'
        elif 300 <= code < 400:
            status_size = '3xx'
        elif 400 <= code < 500:
            status_size = '4xx'
        elif 500 <= code:
            status_size = '5xx'
        if not len(sizes_result[request_parse['ns']]):
            sizes_result[request_parse['ns']] = defaultdict(dict)
        if not len(sizes_result[request_parse['ns']][request_parse['size']]):
            sizes_result[request_parse['ns']][request_parse['size']] = defaultdict(int)

        sizes_result[request_parse['ns']][request_parse['size']][status_size] += 1
        sizes_result[request_parse['ns']][request_parse['size']]['all'] += 1
    else:
        if not len(result[request_parse['ns']]):
            result[request_parse['ns']] = defaultdict(dict)
        if not len(result[request_parse['ns']][request_parse['request_type']]):
            result[request_parse['ns']][request_parse['request_type']] = defaultdict(int)
            result[request_parse['ns']][request_parse['request_type']]['timings'] = []
            result[request_parse['ns']][request_parse['request_type']]['req_timings'] = []
            result[request_parse['ns']][request_parse['request_type']]['cache_timings'] = []

        result[request_parse['ns']][request_parse['request_type']][status] += 1
        result[request_parse['ns']][request_parse['request_type']]['all'] += 1

        if size is not None:
            result[request_parse['ns']][request_parse['request_type']]['size'] += int(size)

        code = int(status)
        # Тийминги апстрима
        if upstream_response_time != '-' and upstream_response_time is not None and code in xrange(199, 399):
            if '_L' == request[-2:] and request_parse['ns'] == 'default':
                if 'L_timings' in result[request_parse['ns']][request_parse['request_type']]:
                    result[request_parse['ns']][request_parse['request_type']]['L_timings'].append(upstream_response_time)
                else:
                    result[request_parse['ns']][request_parse['request_type']]['L_timings'] = [upstream_response_time]
            else:
                result[request_parse['ns']][request_parse['request_type']]['timings'].append(upstream_response_time)

        cache = ['HIT', 'MISS', 'ANY', 'EXPIRED', 'RENEW', 'STALE', 'UPDATING']
        if upstream_cache_status in cache:
            result[request_parse['ns']][request_parse['request_type']][upstream_cache_status] += 1
            # Тaйминги кэша
            if processing_time != '-' and processing_time is not None and code in xrange(199, 399):
                result[request_parse['ns']][request_parse['request_type']]['cache_timings'].append(processing_time)

        # Тaйминги ответа
        if processing_time != '-' and processing_time is not None and code in xrange(199, 399):
            result[request_parse['ns']][request_parse['request_type']]['req_timings'].append(processing_time)

    return result, sizes_result


def print_dashing(result):
    # Вывод результата для dashing`a
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


def print_perl(result):
    # Эта часть кода очень сильно похожа на перловый парсер для совместимости(

    # количество ошибок парсинга
    errors_count = result.pop('err')

    # PERL
    codes = defaultdict(str)
    codes["global_req_timings"] = ""
    codes["global_int_req_timings"] = ""
    codes["global_timings"] = ""
    codes["global_int_timings"] = ""
    codes["global_cache_timings"] = ""
    codes["global_int_cache_timings"] = ""

    cache = ['HIT', 'MISS', 'ANY', 'EXPIRED', 'RENEW', 'STALE']
    for ns, ns_result in result.iteritems():
        for request_type, request_info in ns_result.iteritems():
            # put, get
            _int = ''
            if request_type == 'put':
                _int = "int_"
            elif request_type != 'get':
                continue

            # Регулярка  для http кодов
            http_reg = re.compile("\d\d\d")

            for info, value in request_info.iteritems():
                # Тайминги запросов
                if info == 'req_timings':
                    codes["global_{i}req_timings".format(i=_int)] += " %s" % (' '.join(value))
                    codes["{ns}_{i}req_timings".format(ns=ns, i=_int)] = ' '.join(value)
                # Тайминги апстрима
                elif info == 'timings':
                    codes["global_{i}timings".format(i=_int)] += " %s" % (' '.join(value))
                    codes["{ns}_{i}timings".format(ns=ns, i=_int)] = ' '.join(value)
                # Тайминги кэша
                elif info == 'cache_timings':
                    codes["global_{i}cache_timings".format(i=_int)] += " %s" % (' '.join(value))
                    codes["{ns}_{i}cache_timings".format(ns=ns, i=_int)] = ' '.join(value)
                elif info == 'L_timings' and ns == 'default':
                    codes["{ns}_L_timings".format(ns=ns)] = ' '.join(value)
                # Кэш
                elif info in cache and not _int:
                    codes["{ns}_{cache_info}".format(ns=ns, cache_info=info)] = value
                    if not codes["global_{cache_info}".format(cache_info=info)]:
                        codes["global_{cache_info}".format(cache_info=info)] = value
                    else:
                        codes["global_{cache_info}".format(cache_info=info)] += value
                # Http коды
                elif http_reg.match(info):
                    list_code = {'20x': [x for x in xrange(200, 300)],
                                 '30x': [x for x in xrange(300, 400)],
                                 # '40x': [x for x in xrange(400, 500) if x not in [400, 434, 409]],
                                 '40x': [x for x in xrange(400, 500)],
                                 '50x': [x for x in xrange(500, 600)]
                                 }
                    codes["{ns}_{i}{code}".format(ns=ns, i=_int, code=info)] = value
                    if not codes["global_{i}{code}".format(i=_int, code=info)]:
                        codes["global_{i}{code}".format(i=_int, code=info)] = int(value)
                    else:
                        codes["global_{i}{code}".format(i=_int, code=info)] += int(value)

                    for k, v in list_code.iteritems():
                        if int(info) in v:
                            if not codes["{ns}_{i}{code}".format(ns=ns, i=_int, code=k)]:
                                codes["{ns}_{i}{code}".format(ns=ns, i=_int, code=k)] = value
                            else:
                                codes["{ns}_{i}{code}".format(ns=ns, i=_int, code=k)] += value

                            if not codes["global_{i}{code}".format(i=_int, code=k)]:
                                codes["global_{i}{code}".format(i=_int, code=k)] = int(value)
                            else:
                                codes["global_{i}{code}".format(i=_int, code=k)] += int(value)
                # Остальное
                else:
                    codes["{ns}_{code}".format(ns=ns, code=info)] = value
                    if not codes["global_{i}{code}".format(i=_int, code=info)]:
                        codes["global_{i}{code}".format(i=_int, code=info)] = int(value)
                    else:
                        codes["global_{i}{code}".format(i=_int, code=info)] += int(value)

    for k, v in codes.iteritems():
        if v:
            print k, v
    print "error_parse %d" % int(errors_count)


def main(argv):
    # Ангумента
    options = parseArgs(argv)

    # mds кэш. Если options.cache = False, то cache = [].
    # Иначе в cache массив неймспейсов.
    cache = get_ns_cache(options.cache, options.cache_file)

    result = defaultdict(dict)
    sizes_result = defaultdict(dict)
    result['err'] = 0
    records = sys.stdin
    for line in records:
        line = tskv_parse(line)
        if line is None:
            continue
        request_parse = parse_request(
            line['request'], line['vhost'], cache, options.mds)
        # Запросы, которые не распарсили.
        if request_parse is None:
            result['err'] += 1
            if options.print_error_parse:
                print line
            continue

        # Запись данных в результирующий словарь в зависимости от кода ответа,
        # нейсмспейса и размеров (для аватарницы)
        try:
            result, sizes_result = modify_result(
                result, sizes_result, line, request_parse, options.mds)
        except Exception as e:
            print e
            result['err'] += 1
            if options.print_error_parse:
                print line
            continue

    if options.output == 'perl':
        # Вывод в формате старого парсера
        print_perl(result)

if (__name__ == "__main__"):
    sys.exit(main(sys.argv))

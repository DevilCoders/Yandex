#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import os
import sys
import re
from collections import defaultdict
import xml.etree.cElementTree as ET
from urlparse import parse_qs
import subprocess
import glob


def parseArgs(argv):
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option(
        "--cache", help="[ %default ]", default=False, action='store_true')
    parser.add_option(
        "--cache_file", help="[ %default ]", default='/var/cache/libmastermind/mds.cache')
    parser.add_option(
        "--avatars", help="[ %default ]", default=False, action='store_true')
    parser.add_option(
        "--getonly", help="[ %default ]", default=False, action='store_true')
    parser.add_option(
        "--avatars_sizes", help="[ %default ]", default=False, action='store_true')
    parser.add_option(
        "--mds", help="[ %default ]", default=False, action='store_true')
    parser.add_option(
        "--output", help="[ Dlya obratnoi sovmestimosti so starim scriptom - ebanii perl (perl); Media combaine (grng); Dashing (dashing); Default - %default ]", default='grng')
    parser.add_option(
        "--print_error_parse", help="[ %default ]", default=False, action='store_true')
    parser.add_option(
        "--resizer", help="[ %default ]", default=False, action='store_true')
    parser.add_option(
        "--avatars_old", help="[ %default ]", default=False, action='store_true')
    parser.add_option(
        "--check_https", help="[ %default ]", default=False, action='store_true')
    parser.add_option(
        "--uniq", help="[ %default ]", default=False, action='store_true')
    (options, arguments) = parser.parse_args(argv)
    if (len(arguments) > 1):
        raise Exception("Tool takes no arguments")
    return options


def tskv_parse(logs, avatars=False, mds=False):
    if logs[:5] == "tskv\t":
        fields = ['status', 'request', 'vhost', 'upstream_cache_status', 'upstream_status', 'bytes_sent', 'upstream_response_time', 'request_time', 'scheme']
        row = dict(token.split("=", 1) for token in logs[5:].strip().split("\t") if token.split("=", 1)[0] in fields)
        if not ignore_req(row['request']):
            return row
        else:
            return None
    else:
        return None


def ignore_req(request):
    ignore = ['/ping', '/', '/ping-alive', '/favicon.ico', '-', '/rules.abe', '/robots.txt', '/crossdomain.xml']
    for x in ignore:
        if request == x:
            return True
    if (request.startswith('/timetail') or
            request.startswith('/exec_pattern') or
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


def parse_request(line, vhost, status, cache, getonly=False, avatars=False, avatars_sizes=False, mds=False, resizer=False, avatars_old=False):
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

        # /ge\x16ntity_search/34196/50131933/S122x183
        # /ge\x182.-serp/15207/a992bd97-1859-40c0-b20d-a62cab314e08/s
        if line.startswith('/gex'):
            return None

        if line[-1] == '/':
            line = line[:-1]

        if avatars:
            return avatars_request(line, vhost, status, cache, getonly, avatars, avatars_sizes, avatars_old)
        elif mds:
            return mds_request(line, vhost, cache, mds)
        elif resizer:
            return resizer_request(line)
        else:
            return None
    except:
        return None


def resizer_request(line):
    result = {}
    rt = 'get'
    if line.startswith('/genurl?'):
        # http://resize-int.yandex.net/genurl?url=.......
        ns = 'genurl'
    else:
        ns = 'any'
    result['request_type'] = rt
    result['ns'] = ns
    return result


def avatars_request(line, vhost, status, cache, getonly=False, avatars=False, avatars_sizes=False, avatars_old=False):
    result = {}
    if not avatars_old:
        if 'avatars.mds.yandex.net' not in vhost and 'avatars-int.mds.yandex.net' not in vhost and 'avatars.yandex.net' not in vhost:
            return 'Skip'
    ls = line.split('/')
    # /$type-$namespace
    # /get-yabs_performance
    im = ls[1].split('?')[0]

    # /$im?id=$imagename-$group_id-$namespace&n=$preset&other
    # /i?id=2a000001568e74d34378d960ef18ff5fa6a6-4557-ultra-images&n=13&blur=8
    if im == 'i':
        rt = 'get'
        args = parse_qs(ls[1].split('?')[1])
        if 'id' in args:
            ns = args['id'][0].split('-', 2)[2]
    else:
        rt = ls[1].split('-', 1)[0]
        ns = ls[1].split('-', 1)[1].split('?')[0]

    if getonly and rt != 'get':
        return 'Skip'

    if 'orig-url=' in line:
        # /get-rtb/28673/9568677668965313131/orig?orig-url=https....
        rt = 'put'

    if ns == "autoru-all" and status == "403":
        return 'Skip'

    # В кэше mds аватарные неймспейсы называются avatars-$ns
    mdsns = "avatars-%s" % ns
    if len(cache) != 0 and mdsns not in cache:
        return None

    # Фильтр странных запросов
    # /couplelist-$namespace
    # /statistics-$namespace
    # /ban-$namespace/$group-id/$imagename
    # /delete-$namespace/$group-id/$imagename/$size or /delete-$namespace/$group-id/$imagename
    # /get-$namespace/$group-id/$imagename/$size
    # /get-$namespace/$group-id/$imagename?tumbnail=
    # /getinfo-$namespace/$group-id/$imagename/meta
    # TODO: put, genurlsign
    # if not avatars_old:
    #     if (im != 'i' and ((rt == 'ban' and len(ls) != 4) or
    #                        (rt in ['couplelist', 'statistics'] and len(ls) != 2) or
    #                        (rt == 'delete' and len(ls) not in [4, 5]) or
    #                        (rt in ['get', 'getinfo'] and len(ls) != 5 and 'thumbnail=' not in line) or
    #                        (rt == 'get' and len(ls) != 4 and 'thumbnail=' in line))):
    #         return None

    # Dashing
    # Если используются опция user-thumbnail, то записывается размер
    # thumbnail.
    if avatars_sizes and rt == 'get':
        if ('?thumbnail=' in ls[3]) or im == 'i':
            size = 'thumbnail'
        else:
            size = ls[4].split('?')[0].split('&')[0]
        result['size'] = size
    result['request_type'] = rt
    result['ns'] = ns
    return result


def mds_request(line, vhost, cache, mds=False):
    result = {}
    ls = line.split('/')
    if 's3.mds' in vhost:
        return 'Skip'
    # /get/weather-embed/forecasts-by-geo/24877.xml?embed=1
    # /upload/weather-embed/forecasts/8221.xml?timestamp=1444995769&ioflags=1&embed=1
    if 'weather' in vhost or 'weather-embed' in ls:
        rt = ls[1]
        if rt == 'upload':
            rt = 'put'
        result['request_type'] = rt
        result['ns'] = ls[2]
    # /get/3/209556413.41/0_121645_66967b8b_XXXL.jpg
    elif 'fotki' in vhost:
        result['request_type'] = 'get'
        if '0_STATIC' in line:
            result['ns'] = 'default_static'
        else:
            result['ns'] = 'default'
    elif 'imageprocessor.photo.yandex.net' in vhost:
        result['request_type'] = 'put'
        result['ns'] = 'default'
    # /file-download-info/15332_d70b2896.3418812/320?sign=....
    # /downloadinfo-music/13320/0f56a24d.5726675/2.mp3
    # /download-info/15601_ebd5f848.733567/2?trackId=....
    elif ls[1] in ['download-info', 'downloadinfo-music', 'file-download-info']:
        result['request_type'] = 'downloadinfo'
        result['ns'] = 'music'
    else:
        rt = ''
        if line in ['/traffic', '/ping-alive']:
            result['request_type'] = ls[1]
            result['ns'] = 'no_ns'
        else:
            rt = ls[1].split('-', 1)[0]
            if rt == 'upload':
                rt = 'put'
            result['request_type'] = rt
            ns = ls[1].split('-', 1)[1]
            result['ns'] = ns
            if len(cache) != 0 and ns not in cache:
                return None
    if result['ns'] != 'default':
        return None
    return result


def modify_result(result, sizes_result, requests, line, request_parse, avatars=False, avatars_sizes=False, mds=False, check_https=False, uniq=False):
    status = line.get('status')
    request = line.get('request')
    vhost = line.get('vhost')
    upstream_cache_status = line.get('upstream_cache_status')
    upstream_status = line.get('upstream_status')
    bytes_sent = line.get('bytes_sent')
    upstream_response_time = line.get('upstream_response_time')
    request_time = line.get('request_time')
    scheme = line.get('scheme')
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
        # check_https дописывает перед неймспейсом http(s).
        # используется в ресайзере. К названию неймспейса дописывается http(s).
        # Только для кодов ответа
        if check_https:
            if scheme == 'https':
                ns_tmp = 'https.' + request_parse['ns']
            else:
                ns_tmp = 'http.' + request_parse['ns']
            if not len(result[ns_tmp]):
                result[ns_tmp] = defaultdict(dict)
            if not len(result[ns_tmp][request_parse['request_type']]):
                result[ns_tmp][request_parse['request_type']] = defaultdict(int)

            result[ns_tmp][request_parse['request_type']][status] += 1
            result[ns_tmp][request_parse['request_type']]['all'] += 1

        ns = request_parse['ns']
        if not len(result[ns]):
            result[ns] = defaultdict(dict)
        if not len(result[ns][request_parse['request_type']]):
            result[ns][request_parse['request_type']] = defaultdict(int)
            result[ns][request_parse['request_type']]['timings'] = defaultdict(int)
            result[ns][request_parse['request_type']]['req_timings'] = defaultdict(int)
            result[ns][request_parse['request_type']]['cache_timings'] = defaultdict(int)

        # Коды ответов по уникальным запросам (уже посчитанный запрос
        # складывается в set requests. По ним считаются только чистые коды ответов
        # (без 20x, 30x, etc)
        if uniq and request not in requests:
            requests.add(request)
            result[ns][request_parse['request_type']]["uniq.{0}".format(status)] += 1
            result[ns][request_parse['request_type']]['uniq.all'] += 1

        # Коды ответов
        result[ns][request_parse['request_type']][status] += 1
        result[ns][request_parse['request_type']]['all'] += 1

        if bytes_sent is not None:
            result[ns][request_parse['request_type']]['bytes_sent'] += int(bytes_sent)

        count_timings = True

        # Не учитывать тайминги для аватарницы для ошибок
        if avatars and int(status) >= 400:
            count_timings = False

        # Тийминги апстрима
        if ':' in upstream_response_time:
            upstream_response_times = [x.strip() for x in upstream_response_time.split(':')]
            tmp_time = 0
            tmp_check = False
            for t in upstream_response_times:
                if t != '-':
                    tmp_time += float(t)
                    tmp_check = True
            if tmp_check:
                upstream_response_time = tmp_time
            else:
                upstream_response_time = '-'

        if ',' in upstream_response_time:
            try:
                upstream_response_time = str(sum([float(x.strip()) for x in upstream_response_time.split(',')]))
            except:
                upstream_response_time = '-'

        if count_timings and upstream_response_time != '-' and upstream_response_time is not None:
            if '_L' == request[-2:] and ns == 'default':
                if 'L_timings' in result[ns][request_parse['request_type']]:
                    result[ns][request_parse['request_type']]['L_timings'][upstream_response_time] += 1
                else:
                    result[ns][request_parse['request_type']]['L_timings'] = defaultdict(int)
                    result[ns][request_parse['request_type']]['L_timings'][upstream_response_time] += 1
            else:
                result[ns][request_parse['request_type']]['timings'][upstream_response_time] += 1

        # Коды ответа апстрима
        if ':' in upstream_status:
            upstream_statuses = [x.strip() for x in upstream_status.split(':')]
            upstream_status = upstream_statuses[-1]

        if ',' in upstream_status:
            upstream_status = [x.strip() for x in upstream_status.split(',')][-1]

        if upstream_status != '-' and upstream_status is not None:
            result[ns][request_parse['request_type']]["upstream_{st}".format(st=upstream_status)] += 1
            if upstream_status == '404' and 'url=' in request:
                result[ns][request_parse['request_type']]["upstream_{st}_url".format(st=upstream_status)] += 1

        cache = ['HIT', 'MISS', 'ANY', 'EXPIRED', 'RENEW', 'STALE', 'UPDATING']
        if upstream_cache_status in cache:
            result[ns][request_parse['request_type']][upstream_cache_status] += 1
            # Тaйминги кэша
            if request_time != '-' and request_time is not None and upstream_cache_status == 'HIT':
                result[ns][request_parse['request_type']]['cache_timings'][request_time] += 1

        # Тaйминги ответа
        if count_timings and request_time != '-' and request_time is not None:
            result[ns][request_parse['request_type']]['req_timings'][request_time] += 1

    return result, sizes_result, requests


def print_dashing(result, conf):
    # Вывод результата для dashing`a
    MAX_BUF = 8100
    res = ''
    nss = {}
    for ns, ns_result in result.iteritems():
        requests = 0
        for size_result in ns_result.itervalues():
            requests += size_result['all']
        nss[ns] = requests
    nss = sorted(nss, key=nss.__getitem__, reverse=True)

    for ns in nss:
        ns_result = result[ns]
        tmp = "%s/" % ns
        for size, size_result in ns_result.iteritems():
            if ns in conf:
                if size in conf[ns]:
                    upload = conf[ns][size]
                elif size == 'orig':
                    upload = 'yes'
                else:
                    upload = '404'
            tmp += "%s:%s." % (size.replace('.', '_'), upload)
            for code, code_result in size_result.iteritems():
                tmp += "%s:%d;" % (code, code_result)
            tmp = tmp[:-1] + ','
        if len(res) + len(tmp) < MAX_BUF:
            res += tmp
            res = res[:-1] + '\t'
    print res


def print_perl(result):
    # Эта часть кода очень сильно похожа на перловый парсер для совместимости(

    # количество ошибок парсинга
    errors_count = result.pop('err')

    # PERL
    codes = defaultdict(str)
    codes["global_req_timings"] = defaultdict(int)
    codes["global_int_req_timings"] = defaultdict(int)
    codes["global_timings"] = defaultdict(int)
    codes["global_int_timings"] = defaultdict(int)
    codes["global_cache_timings"] = defaultdict(int)
    codes["global_int_cache_timings"] = defaultdict(int)

    cache = ['HIT', 'MISS', 'ANY', 'EXPIRED', 'RENEW', 'STALE']
    for ns, ns_result in result.iteritems():
        for request_type, request_info in ns_result.iteritems():
            # put, get
            _int = ''
            if request_type == 'put':
                _int = "int_"
            elif request_type == 'get':
                _int = ""
            else:
                _int = "{0}_".format(request_type)

            # Регулярка  для http кодов
            http_reg = re.compile("^\d\d\d$")
            ignore_ns_timings = ['video_frame', 'rtb']
            for info, value in request_info.iteritems():
                # Тайминги запросов
                if info == 'req_timings':
                    if ns not in ignore_ns_timings:
                        for k, v in value.items():
                            if "global_{i}req_timings".format(i=_int) not in codes:
                                codes["global_{i}req_timings".format(i=_int)] = defaultdict(int)
                            codes["global_{i}req_timings".format(i=_int)][k] += v
                    codes["@{ns}_{i}req_timings".format(ns=ns, i=_int)] = ' '.join('{}@{}'.format(key, val) for key, val in value.items())
                # Тайминги апстрима
                elif info == 'timings':
                    if ns not in ignore_ns_timings:
                        for k, v in value.items():
                            if "global_{i}timings".format(i=_int) not in codes:
                                codes["global_{i}timings".format(i=_int)] = defaultdict(int)
                            codes["global_{i}timings".format(i=_int)][k] += v
                    codes["@{ns}_{i}timings".format(ns=ns, i=_int)] = ' '.join('{}@{}'.format(key, val) for key, val in value.items())
                # # Тайминги кэша
                elif info == 'cache_timings':
                    for k, v in value.items():
                        if "global_{i}cache_timings".format(i=_int) not in codes:
                            codes["global_{i}cache_timings".format(i=_int)] = defaultdict(int)
                        codes["global_{i}cache_timings".format(i=_int)][k] += v
                    codes["@{ns}_{i}cache_timings".format(ns=ns, i=_int)] = ' '.join('{}@{}'.format(key, val) for key, val in value.items())
                elif info == 'L_timings' and ns == 'default':
                    codes["@{ns}_L_timings".format(ns=ns)] = ' '.join('{}@{}'.format(key, val) for key, val in value.items())
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

                            # Если неймспейс начинается с http, то его не учитываем в глобальных кодах ответа
                            # Хороший способ выстрелить в ногу, но, надеюсь, реально таких нейсмспейсов не будет))
                            if not ns.startswith('http'):
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
            if 'timings' in k and '@' not in k:
                print '@{} {}'.format(k, ' '.join('{}@{}'.format(key, val) for key, val in v.items()))
            else:
                print k, v
    print "error_parse %d" % int(errors_count)


def print_grng(result):
    # Тут, на самом деле, повторение перлового парсера с заменением _ на .
    # количество ошибок парсинга
    errors_count = result.pop('err')

    codes = defaultdict(str)
    codes["global.req_timings"] = defaultdict(int)
    codes["global.int.req.timings"] = defaultdict(int)
    codes["global.timings"] = defaultdict(int)
    codes["global.int.timings"] = defaultdict(int)
    codes["global.cache.timings"] = defaultdict(int)
    codes["global.int.cache.timings"] = defaultdict(int)

    cache = ['HIT', 'MISS', 'ANY', 'EXPIRED', 'RENEW', 'STALE']
    for ns, ns_result in result.iteritems():
        for request_type, request_info in ns_result.iteritems():
            # put, get
            _int = ''
            if request_type == 'put':
                _int = "int."
            elif request_type == 'get':
                _int = ""
            else:
                _int = "{0}.".format(request_type)

            # Регулярка  для http кодов
            http_reg = re.compile("^\d\d\d$")

            for info, value in request_info.iteritems():
                # Тайминги запросов
                if info == 'req_timings':
                    for k, v in value.items():
                        if "global.{i}req_timings".format(i=_int) not in codes:
                            codes["global.{i}req_timings".format(i=_int)] = defaultdict(int)
                        codes["global.{i}req_timings".format(i=_int)][k] += v
                    codes["@{ns}.{i}req_timings".format(ns=ns, i=_int)] = ' '.join('{}@{}'.format(key, val) for key, val in value.items())
                # Тайминги апстрима
                elif info == 'timings':
                    for k, v in value.items():
                        if "global.{i}upstream_timings".format(i=_int) not in codes:
                            codes["global.{i}upstream_timings".format(i=_int)] = defaultdict(int)
                        codes["global.{i}upstream_timings".format(i=_int)][k] += v
                    codes["@{ns}.{i}upstream_timings".format(ns=ns, i=_int)] = ' '.join('{}@{}'.format(key, val) for key, val in value.items())
                # Тайминги кэша
                elif info == 'cache_timings':
                    for k, v in value.items():
                        if "global.{i}cache_timings".format(i=_int) not in codes:
                            codes["global.{i}cache_timings".format(i=_int)] = defaultdict(int)
                        codes["global.{i}cache_timings".format(i=_int)][k] += v
                    codes["@{ns}.{i}cache_timings".format(ns=ns, i=_int)] = ' '.join('{}@{}'.format(key, val) for key, val in value.items())
                elif info == 'L_timings' and ns == 'default':
                    codes["@{ns}.L_timings".format(ns=ns)] = ' '.join('{}@{}'.format(key, val) for key, val in value.items())
                # Кэш
                elif info in cache and not _int:
                    codes["{ns}.{cache_info}".format(ns=ns, cache_info=info)] = value
                    if not codes["global.{cache_info}".format(cache_info=info)]:
                        codes["global.{cache_info}".format(cache_info=info)] = value
                    else:
                        codes["global.{cache_info}".format(cache_info=info)] += value
                # Http коды
                elif http_reg.match(info):
                    list_code = {'20x': [x for x in xrange(200, 300)],
                                 '30x': [x for x in xrange(300, 400)],
                                 # '40x': [x for x in xrange(400, 500) if x not in [400, 434, 409]],
                                 '40x': [x for x in xrange(400, 500)],
                                 '50x': [x for x in xrange(500, 600)]
                                 }
                    codes["{ns}.codes.{i}{code}".format(ns=ns, i=_int, code=info)] = value
                    if ns.split('.')[0] not in ['http', 'https']:
                        if not codes["global.codes.{i}{code}".format(i=_int, code=info)]:
                            codes["global.codes.{i}{code}".format(i=_int, code=info)] = int(value)
                        else:
                            codes["global.codes.{i}{code}".format(i=_int, code=info)] += int(value)

                    for k, v in list_code.iteritems():
                        if int(info) in v:
                            if not codes["{ns}.{i}{code}".format(ns=ns, i=_int, code=k)]:
                                codes["{ns}.{i}{code}".format(ns=ns, i=_int, code=k)] = value
                            else:
                                codes["{ns}.{i}{code}".format(ns=ns, i=_int, code=k)] += value

                            if ns.split('.')[0] not in ['http', 'https']:
                                if not codes["global.{i}{code}".format(i=_int, code=k)]:
                                    codes["global.{i}{code}".format(i=_int, code=k)] = int(value)
                                else:
                                    codes["global.{i}{code}".format(i=_int, code=k)] += int(value)
                # Остальное
                else:
                    codes["{ns}.{code}".format(ns=ns, code=info)] = value
                    if not codes["global.{i}{code}".format(i=_int, code=info)]:
                        codes["global.{i}{code}".format(i=_int, code=info)] = int(value)
                    else:
                        codes["global.{i}{code}".format(i=_int, code=info)] += int(value)

    for k, v in codes.iteritems():
        if v:
            if 'timings' in k and '@' not in k:
                print '@{} {}'.format(k, ' '.join('{}@{}'.format(key, val) for key, val in v.items()))
            else:
                print k, v
    print "error_parse %d" % int(errors_count)


def parse_xml():
    tree = ET.ElementTree(file='/etc/avatars-mds/avatars-mds.xml')
    result = {}
    for elem in tree.iter(tag='namespace'):
        try:
            for n in elem.findall('handler'):
                ns = n.text
            result[ns] = {}
            for size in elem.iter('size'):
                result[ns][size.attrib['alias']] = size.attrib['upload']
        except:
            pass
    return result


def main(argv):
    # Ангумента
    options = parseArgs(argv)

    # mds кэш. Если options.cache = False, то cache = [].
    # Иначе в cache массив неймспейсов.
    cache = get_ns_cache(options.cache, options.cache_file)

    result = defaultdict(dict)
    sizes_result = defaultdict(dict)
    result['err'] = 0
    logs = sys.stdin
    requests = set()
    for line in logs:
        record = tskv_parse(line)
        if record is None:
            continue
        request_parse = parse_request(
            record.get('request'), record.get('vhost'), record.get('status'), cache, options.getonly, options.avatars, options.avatars_sizes, options.mds, options.resizer, options.avatars_old)
        # Запросы, которые не распарсили.
        if request_parse is None:
            result['err'] += 1
            if options.print_error_parse:
                print line
            continue
        elif request_parse == 'Skip':
            continue
        # Запись данных в результирующий словарь в зависимости от кода ответа,
        # нейсмспейса и размеров (для аватарницы)
        try:
            result, sizes_result, requests = modify_result(
                result, sizes_result, requests, record, request_parse, options.avatars, options.avatars_sizes, options.mds, options.check_https, options.uniq)
        except:
            result['err'] += 1
            if options.print_error_parse:
                print line
            continue
    if options.output == 'dashing' and options.avatars_sizes:
        # Вывод результата для dashing`a
        ns_config = parse_xml()
        print_dashing(sizes_result, ns_config)
    elif options.output == 'perl':
        # Вывод в формате старого парсера
        print_perl(result)
    elif options.output == 'grng':
        print_grng(result)


if (__name__ == "__main__"):
    sys.exit(main(sys.argv))

#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import sys
import re
from collections import defaultdict

# request_id = re.compile('(?<=fastcgi-avatars-mds:|fastcgi-resizer:)([\d\w]+)(?=: ): (.*$)')
request_id = re.compile(
    '(avt:|fastcgi-resizer:)([\d\w]+)(?=: ): (.*$)')
cache = re.compile('(?<=description=")[A-Z]+(?=")')
cache_time_re = re.compile('(?<=spent-time=")\d+(?=ms")')
url_re = re.compile('(?<=url=").*(?=")')
use_zora = re.compile('(?<=use_zora=")(yes|no)(?=";)')

def parseArgs(argv):
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option(
        "--resizer", help="[ %default ]", default=False, action='store_true')
    (options, arguments) = parser.parse_args(argv)
    if (len(arguments) > 1):
        raise Exception("Tool takes no arguments")
    return options


def parse_resizer(line):
    try:
        if ' from cache";' in line:
            ct = cache_time_re.findall(line)
            if len(ct):
                cache_time = '%.3f' % (int(ct[0]) / 1000)
            # HIT MISS ..
            matches = cache.findall(line)
            if len(matches):
                c = matches[0]
                if c == 'HIT' and 'orig from cache' in line:
                    c = 'origHIT'
            return {'cache_timings': cache_time, 'cache': c}
        elif 'operation-title="download file";' in line:
            matches = use_zora.findall(line)
            if len(matches):
                z = matches[0]
            return {'use_zora': z}
    except:
        return None


def parse(line):
    try:
        if 'process request: namespace=' in line:
            # process request: namespace=avatars-ynews
            return {'ns': line.split('namespace=')[1][8:]}
        elif 'process request: done;' in line:
            # process request: done;
            # url="/get-autoru-all/31708/8461d45cfd27d8494a5a78532c9fbdb3/small";
            # spent-time=19ms
            request_time = '%.3f' % (
                float(line.split('spent-time=')[1][:-2]) / 1000)
            matches = url_re.findall(line)
            if len(matches):
                url = matches[0]
            ls = url.split('/')
            # /$type-$namespace
            if 'orig-url=' in line:
                # /get-rtb/28673/9568677668965313131/orig?orig-url=https....
                rt = 'put'
            else:
                # /get-yabs_performance
                rt = ls[1].split('-', 1)[0]
            ns = ls[1].split('-', 1)[1]
            return {'request_timings': request_time, 't': rt, 'ns': ns}
        elif 'operation-title="get cache object";' in line:
            # [2015-11-19T17:40:28.511459+03:00 ] info:
            # fastcgi-avatars-mds:2e5f0d1d46b0e44e1ea199b4e88b5ebb:
            # operation-title="get cache object";
            # key="1/21438/video_thumb.46ff219670add61ae65854c43caaf2f01c700ba7.normal";
            # group="2"; expiration-time="120s"; description="EXPIRED";
            # spent-time="0ms";
            ct = cache_time_re.findall(line)
            if len(ct):
                cache_time = '%.3f' % (int(ct[0]) / 1000)
            # HIT MISS ..
            matches = cache.findall(line)
            if len(matches):
                c = matches[0]
            return {'cache_timings': cache_time, 'cache': c}
        elif 'imagemagick is processed;' in line:
            # imagemagick is processed; spent-time=1110ms
            imagemagick_time = '%.3f' % (
                float(line.split('spent-time=')[1][:-2]) / 1000)
            return {'imagemagick_timings': imagemagick_time}
        elif 'libmagic: content-type="image/jpeg";' in line:
            # libmagic: content-type="image/jpeg"; spent-time=0ms
            libmagic_time = '%.3f' % (
                float(line.split('spent-time=')[1][:-2]) / 1000)
            return {'libmagic_timings': libmagic_time}
        elif 'external processor error' in line:
            if 'description="10 Bad image sizes"' in line:
                cv = '10_Bad_image_sizes'
            elif 'description="4 Bad binary representation of image"' in line:
                cv = '4_Bad_binary_representation_of_image'
            elif 'description="No image"' in line:
                cv = 'No_image'
            elif 'description=""' in line:
                cv = 'unknown'
            elif 'Timeout was reached' in line:
                cv = 'Timeout_was_reached'
            elif 'Couldn\'t connect to server' in line:
                cv = 'Couldnt_connect_to_server'
            elif ('current_application' and 'is_unavailable') in line:
                cv = 'application_is_unavailable'
            else:
                cv = 'other'
            return {'cverrs': cv}
        elif ('process the request with' in line) and ('attempt' in line):
            try:
                at = line.split()[-1]
                attempt = int(at.split('=')[-1])
                if attempt > 1:
                    return {"attempt_{0}".format(attempt): attempt}
            except:
                pass
        elif ('process imagemagick: done; spent-time=' in line):
            # check size upload=lazy
            return {'lazy': 'lazy'}
        elif ('file was downloaded; spent-time=' in line):
            download_time = '%.3f' % (
                float(line.split('spent-time=')[1][:-2]) / 1000)
            return {'download_timings': download_time}
        elif ('external processor' and 'get response; spent-time=') in line:
            cv_download_time = '%.3f' % (
                float(line.split('spent-time=')[1][:-2]) / 1000)
            return {'cv_timings': cv_download_time}
    except:
        return None


def modify_result(res, resizer=False):
    result = defaultdict(dict)
    result['global'] = defaultdict(dict)
    result['global']['all'] = defaultdict(int)

    for val in res.values():
        if 'ns' in val:
            ns = val.pop('ns')
        else:
            ns = 'unknown'
        if 't' in val:
            request_type = val.pop('t')
        else:
            request_type = 'unknown'

        if not len(result[ns]):
            result[ns] = defaultdict(dict)
        if not len(result[ns][request_type]):
            result[ns][request_type] = defaultdict(int)
            result[ns][request_type]['cache_timings'] = []
            if not resizer:
                result[ns][request_type]['request_timings'] = []
                result[ns][request_type]['libmagic_timings'] = []
                result[ns][request_type]['imagemagick_timings'] = []
                result[ns]['all']['download_timings'] = []
                result[ns]['all']['cv_timings'] = []

        if not len(result['global'][request_type]):
            result['global'][request_type] = defaultdict(int)
            result['global'][request_type]['cache_timings'] = []
            if not resizer:
                result['global'][request_type]['request_timings'] = []
                result['global'][request_type]['libmagic_timings'] = []
                result['global'][request_type]['imagemagick_timings'] = []
                result['global']['all']['download_timings'] = []
                result['global']['all']['cv_timings'] = []

        for k, v in val.iteritems():
            if 'timings' in k:
                # Для них не строим графики по типу запросов
                if k in ['download_timings', 'cv_timings']:
                    result[ns]['all'][k].append(v)
                    result['global']['all'][k].append(v)
                else:
                    result[ns][request_type][k].append(v)
                    result['global'][request_type][k].append(v)
            elif k in ['cache', 'cverrs', 'use_zora']:
                result[ns][request_type]["{}_{}".format(k, v)] += 1
                result['global'][request_type]["{}_{}".format(k, v)] += 1
            elif 'attempt' in k or k in ['lazy']:
                result[ns][request_type][k] += 1
                result['global'][request_type][k] += 1
                result['global']['all'][k] += 1

    return result


def main(argv):
    options = parseArgs(argv)

    by_request_id = defaultdict(dict)
    for line in sys.stdin:
        line = line.strip()
        matches = request_id.findall(line)
        if len(matches):
            ri = matches[0]
            if options.resizer:
                p = parse_resizer(ri[2])
            else:
                p = parse(ri[2])
            if p:
                by_request_id[ri[1]].update(p)

    result = modify_result(by_request_id, options.resizer)

    # print
    for ns, value in result.iteritems():
        for rt, val in value.iteritems():
            for k, v in val.iteritems():
                if 'timings' in k:
                    if len(v) == 0:
                        v = '0.000'
                    else:
                        v = ' '.join(v)
                print "{}.{}.{} {}".format(ns, rt, k, v)

if (__name__ == "__main__"):
    sys.exit(main(sys.argv))

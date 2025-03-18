#!/usr/bin/env python

from mapreducelib import MapReduce, Record
import re
import random
import hashlib

def convert_sessions(rec):
    chunks = rec.value.split('\t\t')
    q = None
    pg = '0'
    lr = ''
    rid = ''
    domain = ''
    uil = ''
    extra_flags = ''

    for keyval in chunks[0].split('\t'):
        if not keyval:
            continue
        key_with_value = keyval.split('=', 1)
        if len(key_with_value) < 2:
            continue
        key, value = key_with_value
        if key == 'query':
            q = value.strip()
        elif key == 'user-region':
            lr = value
        elif key == 'page':
            pg = value
        elif key == 'reqid':
            rid = value
        elif key == 'dom-region':
            domain = value
        elif key == 'uil':
            uil = value
        elif key == 'msp' or key == 'reask' or key == 'full-request' or \
             key == 'testslot' or key == 'exps' or key == 'test-buckets' or \
             key == 'test-ids' or key == 'exp_config_version':
            extra_flags += ('%s=%s\t' % (key, value))

    if not q:
        return

    hsh = hashlib.md5()
    hsh.update(q + '\t' + uil + '\t' + domain)
    urls = []

    for chunk in chunks[1:]:
        url = None
        src = None
        for keyval in chunk.split('\t'):
            if not keyval:
                continue
            key, value = keyval.split('=', 1)
            if key == 'url':
                url = value
            elif key == 'source':
                src = value
        if not url or not src:
            continue
        urls.append((src, url))
        hsh.update('\t' + src + ':' + url)

    if urls:
        yield Record(hsh.hexdigest(), rid, 'dom=%s\t%suil=%s\tq=%s\tlr=%s\tp=%s\t%s' % (domain, extra_flags, uil, q, lr, pg, '\t'.join(['url=%s:%s' % (url[0], url[1]) for url in urls])), tableIndex = 0)

def extract_sessions(uid, recs):
    seen_queries = set()
    if not uid.startswith('y'):
        return
    if random.random() < 0.993:
        return

    for rec in recs:
        chunks = rec.value.split('\t\t')
        q = None
        pg = '0'
        lr = ''
        rid = ''
        domain = ''
        uil = ''
        reqrelev = ''

        bad_rec = False
        filtered_rec = ''
        for keyval in chunks[0].split('\t'):
            if bad_rec:
                break
            if keyval.find('=') < 0:
                continue
            key, value = keyval.split('=', 1)
            if key == 'type' and value != 'REQUEST':
                bad_rec = True
                break
            elif key == 'service' and value != 'www.yandex':
                bad_rec = True
                break
            elif key == 'ui' and value != 'www.yandex':
                bad_rec = True
                break
            elif key == 'query':
                q = value.strip()
            elif key == 'user-region':
                lr = value
            elif key == 'page':
                bad_rec = value != '0'
                pg = value
            elif key == 'reqid':
                rid = value
            elif key == 'dom-region':
                bad_rec = value not in ('ru', 'com', 'ua')
                domain = value
            elif key == 'uil':
                bad_rec = value not in ('ru', 'en', 'uk')
                uil = value
            elif key == 'reqrelev':
                reqrelev = value
            if key != 'search-props':
                filtered_rec += keyval + '\t'

        if bad_rec or not rid:
            continue

        urls = []

        for chunk in chunks[1:]:
            filtered_rec += '\t'
            url = None
            good = True
            for keyval in chunk.split('\t'):
                key, value = keyval.split('=', 1)
                if key == 'url':
                    url = value
                if key == 'lang':
                    good = good and value in ('ru', 'ua', 'en')
                if key != 'markers':
                    filtered_rec += keyval + '\t'
            if not url or not good:
                continue
            urls.append(url)

        if not urls:
            continue

        qkey = q + '\t' + pg
        if qkey in seen_queries:
            continue
        seen_queries.add(qkey)

        yield Record(rec.key, rec.subkey, filtered_rec)

if __name__ == '__main__':
    MapReduce.useDefaults(mrExec='./mapreduce', server = 'cedar00.search.yandex.net:8013', saveSource = True, verbose = True, auxExecArguments=['-opt', 'net_table=ipv6', '-stderrlevel', '5'] )
    dates = ['20141001']
    result_prefix = 'pzuev/random_sample_seglearn'

    intermediates = []
    for dt in dates:
        src = 'user_sessions/' + dt
        dst = '%s_raw_%s' % (result_prefix, dt)
        intermediates.append(dst)
        MapReduce.runReduce(extract_sessions, srcTable=src, dstTable=dst, appendMode = False)
        MapReduce.sortTable(srcTable=dst, dstTable = dst)
    MapReduce.mergeTables(srcTables=intermediates, dstTable = '%s_raw' % result_prefix)
    dst = result_prefix
    MapReduce.runMap(convert_sessions, srcTable='%s_raw' % result_prefix, dstTable=dst, appendMode = False)
    MapReduce.sortTable(srcTable = dst, dstTable = dst)

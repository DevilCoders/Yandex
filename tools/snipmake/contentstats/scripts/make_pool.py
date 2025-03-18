#!/usr/bin/env python
import sys
import util
import random


def remove_scheme(url):
    if url.startswith('http://'):
        url = url[7:]
    elif url.startswith('https://'):
        url = url[8:]
    return url

def remove_http(url):
    if url.startswith('http://'):
        url = url[7:]
    return url

def all_group_names(url):
    url = remove_scheme(url)
    qmark = url.find('?')
    if qmark >= 0:
        url = url[:qmark]
    result = []
    pos = 0
    while len(result) < 3 and pos < len(url):
        pos = url.find('/', pos)
        if pos < 0:
            result.append(url)
            break
        else:
            result.append(url[:pos])
            pos += 1
    result.reverse()
    return result

if __name__ == '__main__':
    random.seed(2342352351)
    group_names = set(util.read_lines('groups.list'))
    poolf = open('hamster_pool.txt', 'w')
    urlf = open('hamster_urls.txt', 'w')
    found_urls = set()

    for reqid, params, queries, urls in util.read_sample_table(sys.stdin):
        for url, owner, source, query_index in urls:
            url_groups = all_group_names(url)
            group_name = None
            for g in url_groups:
                if g in group_names:
                    group_name = g
                    break
            if not group_name:
                continue
            if random.random() > 0.9:
                print >>poolf, '\t'.join( (queries[query_index], '(url:' + remove_http(url) + ')', params['lr']) )
                if url not in found_urls:
                    print >>urlf, url
                    found_urls.add(url)

    poolf.close()
    urlf.close()

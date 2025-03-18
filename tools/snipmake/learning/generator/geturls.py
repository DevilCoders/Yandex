#!/usr/local/bin/python
# -*- coding: utf8 -*-
from sys import argv, __stdout__, path
from random import shuffle
from urlparse import urlparse
from optparse import OptionParser
from os.path import dirname
path.insert(0, dirname(dirname(__file__)))
from common import readConfig

def getcount(settings, count):
    percents = {}
    config = readConfig(settings)
    config['task_keys']
    config['task_values']
    for k in range(len(config['task_keys'])):
        percents[config['task_keys'][k]] = float(config['task_values'][k])
    psum = float(sum(percents.itervalues()))
    counts = {}
    counts.update(map(lambda x: [x[0], int(x[1] / psum * count)], percents.items()))
    return counts

def host(url):
    hostname = urlparse(url).hostname
    return '.'.join(hostname.split('.')[-2:])

_BANNED_HOSTS = ['yandex.ru', 'wikipedia.org', 'vkontakte.ru']

def geturls(infile, count_urls, toprint, done_tasks, timeline, min_weight = 0, outfile = None, verbose = False):
    data = open(infile, 'r').readlines()
    bad_urls = []
    if done_tasks:
        for done_files in done_tasks:
            bad_urls += map(lambda x: x.split('\t')[1] +'\t'+ x.split('\t')[3], open(done_files).readlines())
    indexes = range(len(data))
    shuffle(indexes)
    outfile = open(outfile, 'w') if outfile else __stdout__
    result = {}
    problems = {'assessed':0, 'weight':0, 'relevance':0, 'date':0, 'duplicate':0, 'banned':0, 'length':0}
    def getone(togen, string):
        query,url,relevance,date,region,assid,weight = string.split('\t')
        if query + '\t' + url in bad_urls:
            if verbose:
                print 'assessed', query + '\t' + url
            problems['assessed'] += 1
            return None
        if int(weight) < min_weight:
            if verbose:
                print 'weight', weight.strip()
            problems['weight'] += 1
            return None
        if relevance not in togen or togen[relevance] <= 0:
            if verbose:
                print 'relevance', relevance, togen
            problems['relevance'] += 1
            return None
        if int(date) < timeline:
            if verbose:
                print 'date', date
            problems['date'] += 1
            return None
        if query in result:
            #def filterquery(q):
            #    return True if len(q)==2 else False
            #if len(filter(filterquery,result.itervalues())) > 0: #count_urls/3:
            #    return None
            if len(result[query]) > 1:
                if verbose:
                    print 'query two', query
                problems['duplicate'] += 1
                return None
            if host(url) == host(result[query][0]):
                if verbose:
                    print 'query host', query, url
                problems['duplicate'] += 1
                return None
        if host(url) in _BANNED_HOSTS:
            if verbose:
                print 'banned', url
            problems['banned'] += 1
            return None
        if len(QLENS) > 0 and len(query.split(' ')) in QLENS:
            if verbose:
                print 'length', query
            problems['length'] += 1
            return None
        return query,region,url,relevance
    
    m = 0
    #k = 0
    k = options.baseid
    #for k in range(count_urls):
    while sum(toprint.itervalues()) > 0:
        p = None
        while p is None:
            if m == len(indexes):
                break
            p = getone(toprint, data[indexes[m]])
            m += 1
        if m == len(indexes):
            break
        query, region, url, relev = p
        if query in result:
            result[query].append(url)
        else:
            result[query] = [url]
        toprint[relev] -= 1
        print >> outfile, '\t'.join([str(k), query, region, url, relev])
        k += 1
    print '\n'.join(map(lambda x: 'problem: ' + x[0] + ': ' + str(x[1]), problems.iteritems()))
    print m, 'tasks read,', sum(map(len, result.itervalues())), 'tasks built' 

if __name__=="__main__":
    usage = 'geturls.py <config> <gulin3> <count> [-t <timeline>] [-d <done_tasks>]* [-o <output>] [-w <weight>] [-b baseid] [-v]'
    parser = OptionParser(usage)
    parser.add_option('-d', '--done', dest = 'done_tasks', action = 'append')
    parser.add_option('-t', '--timeline', action = 'store', type = 'int')
    parser.add_option('-o', '--output', action = 'store')
    parser.add_option('-v', '--verbose', action = 'store_true')
    parser.add_option('-w', '--weight', action = 'store', type = 'int')
    parser.add_option('-b', '--baseid', action = 'store', type = 'int', default = 0)
    (options, args) = parser.parse_args()
    if len(args) != 3:
        parser.error(usage)
        exit()
    toprint = getcount(args[0], int(args[2]))
    timeline = options.timeline if options.timeline else 20091001
    weight = options.weight if options.weight else 0
    QLENS = []
    geturls(args[1], int(args[2]), toprint, options.done_tasks, timeline, weight, options.output, options.verbose)

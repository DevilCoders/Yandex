#!/usr/local/bin/python
# -*- coding: utf8 -*-

from sys import argv, path
from optparse import OptionParser
from os.path import dirname, sep
path.insert(0, sep.join([dirname(dirname(__file__)), 'snipmetricsui', 'snipmetricsview']))
from common_utils.stringUtils import toUnicode, toStr
from common_utils.sengines import yandexSerp, yandexNewSerp
from common_utils.html import removeHtml
from candidate_parser import SnippetsData

def get_snippet(serp, task, serp_type = "yandex", param = ""):
    id, query, region, url = task
    full_query = query + " url:" + url
    #param += '&lr=' + region
    #print full_query
    if serp_type == "yandex":
        snips = yandexNewSerp(full_query, yandsite = serp+"/yandsearch/", additionalParams = param)
    elif serp_type == "basesearch":
        snips = yandexSerp(full_query, yandsite = serp+"/yandsearch/", additionalParams = param)
    else:
        print "Unknown serp type:", serp_type
    
    result = []
    for snip in snips:
        result.append((id, toStr(removeHtml(snip.title.replace("\n","")), "utf8"), toStr(removeHtml(snip.body.replace("\n","")), "utf8")))
    return result

if __name__=="__main__":
    usage = "usage: %prog candidates [options]"
    parser = OptionParser(usage)
    parser.add_option("-s", "--serp", action="store", default = "hamster.yandex.ru", 
                      help="SERP to compare snippets with")
    parser.add_option("-p", "--param", action="store", default = "",
                      help="additional parameters for snippets dump")
    parser.add_option("-t", "--type", action="store", default = "yandex",
                      help="serp type: yandex(default) / basesearch")
    (options, args) = parser.parse_args()
    
    if len(args) != 1:
        parser.error("incorrect number of arguments")
        exit()
    candidatesfile = open(args[0])
    
    def clear(snippet):
        return snippet.replace('<strong>', '').replace('</strong>', '').strip().replace('&gt;', '>').replace('&lt;', '<').replace('&#39;', '\'').replace('&quot;', '"').replace('  ', ' ').replace('... ', '...')
    
    candidates = {}
    for line in candidatesfile:
        cand = SnippetsData()
        cand.LoadSimple(line, 10)
        if cand.id not in candidates:
            candidates[cand.id] = {'task':(cand.id, cand.query, cand.region, cand.url), 'Finals':[], 'all':[]}
        if cand.algorithm == 'Finals':
            candidates[cand.id]['Finals'].append(clear(cand.snippet))
        candidates[cand.id]['all'].append(clear(cand.snippet))
    print 'tasks:', len(candidates)
    
    finals = set()
    all = set()
    none = set()
    for candidate in candidates.itervalues():
        serp = get_snippet(options.serp, candidate['task'], options.type, "&rd=0&relev=dq=0" + options.param)
        ffin = 0
        fall = 0
        for id, title, snippet in serp:
            snippet = snippet.strip().replace('  ', ' ').replace('... ', '...')
            if snippet in candidates[id]['Finals']:
                ffin = 1
            if snippet in candidates[id]['all']:
                fall = 1
        if ffin==1:
            finals.add(id)
        if fall==1:
            all.add(id)
            res = '\tOK'
        else:
            none.add(id)
            res = '\tNotFound'
            #print '\n\n', snippet, '\n', candidates[id]['Finals'][0]
        print '\t'.join(candidate['task']) + res
    
    print 'found:', len(all)
    print 'found in finals:', len(finals)
    print 'not found:', len(none)
    
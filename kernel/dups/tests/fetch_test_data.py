#! /usr/bin/env python
# -*- coding: utf-8 -*-

import collections
import optparse
import os
import re
import sys
import time
import urllib
import xml.dom.minidom

try:
    import json
except ImportError:
    import simplejson as json

def callWithRetry(func, max_attempts=3, delay=1):
    attempts = 0
    while True:
        try:
            return func()
        except:
            attempts += 1
            if attempts >= max_attempts:
                raise
            time.sleep(delay)

def getWizardedQuery(xml_host, query, region_id, extra):
    url = 'http://' + xml_host + '/xmlsearch?text=' + urllib.quote_plus(query.strip()) + '&lr=' + str(int(region_id)) + '&full-query=1' + extra
    data = urllib.urlopen(url).read()
    try:
        dom = xml.dom.minidom.parseString(data)
    except:
        raise Exception('Bad responce at ' + url + ':\n' + data)

    q = dom.getElementsByTagName('full-query')
    if len(q) > 0:
        q = q[0]
        if len(q.childNodes) == 1:
            result = q.childNodes[0].data
        else:
            raise Exception(url + ': bad full-query tag')
    else:
        raise Exception(url + ': no full-query tag')

    result = result.split('yandsearch?')
    if len(result) != 2:
        raise Exception(url + ': no yandsearch in full-query')
    else:
        return result[1]

def parseXMLResults(dom, factor_names):
    response = dom.getElementsByTagName('response')
    if len(response) != 1:
        raise Exception('No <response> tag')

    groupings = response[0].getElementsByTagName('grouping')
    if len(groupings) == 0:
        return []

    results = []

    for group in groupings[0].getElementsByTagName('group'):
        domain = ''
        try:
            domain = group.getElementsByTagName('categ')[0].getAttribute('name')
        except:
            domain = ''

        for doc in group.getElementsByTagName('doc'):
            def get(elem, name):
                res = []
                for node in elem.getElementsByTagName(name):
                    for n in node.childNodes:
                        s = ''
                        if n.nodeType == n.TEXT_NODE:
                            res.append(n.data.encode('utf-8'))
                        if n.nodeName == 'hlword':
                            res.append(n.childNodes[0].data.encode('utf-8'))
                return re.sub('[\n ]+', ' ', ' '.join(res))

            def get_clones():
                res = []
                try:
                    for elem in doc.getElementsByTagName('clon'):
                        res.append(elem.childNodes[0].data)
                except:
                    None
                return {'clon':res}

            def get_passages():
                res = []
                for passage in doc.getElementsByTagName('passages'):
                    res.append(get(passage, 'passage'))
                return res

            try:
                factors = {}
                for x, y in re.findall('<tr><td>([^<]+)</td><td>(?:<nobr>)?([0-9.]+)', get(doc, '_RelevFactors')):
                    if x in factor_names or 'all' in factor_names:
                        factors[x] = y

                doc = {
                    'relevance' : get(doc, 'relevance'),
                    'url' : get(doc, 'url'),
                    'title': get(doc, 'title'),
                    'domain' : domain,
                    'passages' : get_passages(),
                    'attributes' : get_clones(),
                    'factors' : factors,
                    'size' : get(doc, 'size'),
                }

                results.append(doc)
            except:
                sys.stderr.write('Skipped a document: %s\n' % sys.exc_value)

    return results

def fixCGI(str, name, val):
    if not len(val):
        return str

    def append(match):
        if match.group(0) != '':
            return match.group(0) + ';' + val
        else:
            return '&' + name + '=' + val

    return re.sub('&' + name + '=[^&]*', append, str)

def getFactorNames(options):
    factor_names = {}
    for factor in options['factors'].split(','):
        factor_names[factor] = 0
    return factor_names

def fetchXMLResults(query, region_id, options):
    grouping = options['grouping']
    params = '&g=' + grouping + '&ag=' + grouping.split('.')[1] + options['gta'] + options['extra'] # grouping and rd params
    wizquery = callWithRetry(lambda: getWizardedQuery(options['upper-host'], query, region_id, params))

    for param in 'g ms ag mslevel gta reqid'.split():
        wizquery = re.sub('&' + param + '=[^&]*', '', wizquery)

    if 'rearr' in options:
        wizquery = fixCGI(wizquery, 'rearr', urllib.quote_plus(options['rearr']))

    if 'relev' in options:
        wizquery = fixCGI(wizquery, 'relev', urllib.quote_plus(options['relev']))

    wizquery += params
    if 'relevance' in options or 'ask-factors' in options:
        wizquery += '&dbgrlv=da'  # with relevance

    if 'factors' in options:
        wizquery += '&gta=_RelevFactors'            # request factor values
        wizquery += '&waitall=da&timeout=10000000'  # search timeout 10 sec

    wizquery += '&xml=da'  # XML output

    url = 'http://' + options['meta-host'] + '/yandsearch?' + wizquery

    if 'url' in options:
        print url

    return callWithRetry(lambda: urllib.urlopen(url).read())


if __name__ == '__main__':
    options = {}
    shortOptions = {}

    def addOptionImpl(option, opt_str, value, parser):
        opt = opt_str.strip('-')
        if opt in shortOptions:
            opt = shortOptions[opt]

        if value is not None:
            options[opt] = value
        else:
            options[opt] = 1

    def addOption(parser, long, short, t, d, h):
        name = long.strip('-')
        if t is not None and d is not None:
            options[name] = d
        shortOptions[short.strip('-')] = name
        parser.add_option(long, short, action='callback', callback=addOptionImpl, type=t, default=d, help=h, metavar='VAL')

    parser = optparse.OptionParser(
        description='Reads from stdin lines containing tab-separated UTF-8 query and region id, performs wizarding and search, writes to stdout factor values of found documents in JSON or tab-separated values format.'
    )

    gta = '&gta=_Markers&gta=_Region&gta=_PassagesType&gta=_IsFake&gta=_SearcherHostname&gta=_MetaSearcherHostname&gta=_Shard&gta=_Factor_DocLen&gta=_HilitedUrl&gta=BaseType&gta=lang&gta=geoa&gta=geo&gta=noarchive&gta=_HeadlineSrc&gta=pp&gta=catalog&gta=extended&gta=vertis&gta=review&gta=torrent_film&gta=vthumb&gta=videorating&gta=blogs&gta=auto&gta=moikrug&gta=photorecipe&gta=social&gta=_UrlMenu&gta=market&gta=_DateSource&gta=_DateAccuracy&gta=sitelinks&gta=ruwiki_sitelinks&gta=enwiki_sitelinks&gta=kino_sitelinks&gta=person_card_links&gta=paradoc&gta=clon'

    addOption(parser, '--upper-host', '-u', 'string', 'xmlsearch.yandex.ru', 'xml uppersearch host for obtaining wizarded queries. E.g. xmlsearch.<jail>.yandex.ru. (default: %default)')
    addOption(parser, '--meta-host', '-m', 'string', 'mmb00:8031', 'middle metasearch host (default: %default)')
    addOption(parser, '--grouping', '-g', 'string', '1.d.30.1.-1.0.0.-1.rlv.0..2.2', 'grouping parameters (default: %default)')
    addOption(parser, '--gta', '-G', 'string', gta, 'gta parameters (default: lots of them, ask with --default-gta)')
    addOption(parser, '--factors', '-F', 'string', '', 'print factor values for given factor names (separated by comma) (default: %default)')
    addOption(parser, '--extra', '-e', 'string', '', 'extra cgi params (default: %default)')
    addOption(parser, '--relevance', '-r', None, None, 'print relevance')
    addOption(parser, '--ask-factors', '-f', None, None, 'request factor values')
    addOption(parser, '--rearr', '', 'string', '', 'add values to rearr=')
    addOption(parser, '--relev', '', 'string', '', 'add values to relev=')
    addOption(parser, '--xml-only', '-x', None, None, 'only download XML results')
    addOption(parser, '--parse-only', '-p', 'string', None, 'only parse xmls from given filenames')
    addOption(parser, '--json', '-j', None, None, 'print result in JSON format')
    addOption(parser, '--url', '-U', None, None, 'print searched URL')

    parser.parse_args()

    xmlResults = ''
    if 'parse-only' in options:
        file = open(options['parse-only'])
        xmlResults = file.read()
    else:
        for input_line in sys.stdin:
            input_fields = input_line.rstrip('\n').split('\t')
            if len(input_fields) != 2:
                raise Exception('Error at input line %d: expected to get two tab-separated fields.\n' % (query_id + 1))

            xmlResults = fetchXMLResults(input_fields[0], input_fields[1], options)

    results = []
    if 'xml-only' in options:
        print xmlResults
    else:
        results = parseXMLResults(xml.dom.minidom.parseString(xmlResults), getFactorNames(options))

    if 'json' in options:
        json.dump(results, sys.stdout, ensure_ascii=False, sort_keys=True, indent=4)
    else:
        n = 0
        for doc in results:
            sys.stdout.write('\t'.join([str(n), doc['url']]) + '\n')
            sys.stdout.flush()
        n += 1

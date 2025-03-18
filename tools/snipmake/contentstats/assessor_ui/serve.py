#!/usr/bin/env python
#coding:utf-8

import sys
import os.path
import urllib
import urllib2
import re
import json
from cStringIO import StringIO
from threading import RLock

from flask import Flask, render_template, request, jsonify

sys.path.append(os.path.join(os.path.dirname(__file__), '../../steam/pyhtml_snapshot'))

from cache import Cache
from pyhtml_snapshot import FetchedDoc, AsyncFetchResult, ZoraFetcher, WebContentParser, CanRetry, GetEncodingName
import proxylib

charrepl_re = re.compile(u'[<>]', re.UNICODE)

def escaped_jsonify(val):
    def replacefn(m):
        c = m.group(0)[0]
        if c == '<':
            return u'\\u0060'
        elif c == '>':
            return u'\\u0062'
        else:
            return c

    return charrepl_re.sub(replacefn, json.dumps(val, ensure_ascii=False, encoding='utf-8'))

def load_urllist():
    result = []
    with open('url_groups.txt') as inf:
        for line in inf:
            line = line.strip().decode('utf-8')
            if not line:
                continue
            group, url = line.split(u'\t', 1)
            result.append((group, url))
    return result

class Vote(object):
    __slots__ = ['url', 'group', 'text', 'hsh', 'state']

    def __init__(self):
        self.state = u'none'

    def to_js_object(self):
        return {
            u'url' : self.url,
            u'group' : self.group,
            u'sent' : {
                u'hash' : self.hsh,
                u'text' : self.text,
            },
            u'state' : self.state
        }

def parse_vote(jsVote):
    vote = Vote()
    sent = jsVote.get(u'sent', {})
    urlgroup = jsVote.get(u'group', None)
    hsh = sent.get(u'hash', None)
    text = sent.get(u'text', u'')
    if not urlgroup or not hsh:
        return None
    vote.url = jsVote.get(u'url')
    vote.group = urlgroup
    vote.hsh = hsh
    vote.text = text
    vote.state = jsVote.get(u'state', STATE_NONE)
    return vote

class VoteDB(object):
    FILE_NAME = 'votes.json'
    mutex = None
    votes = {}
    sealed = set()

    def __init__(self):
        self.mutex = RLock()
        if not os.path.exists(self.FILE_NAME):
            return
        with open(self.FILE_NAME, 'r') as inf:
            for line in inf:
                line = line.strip()
                if not line:
                    continue
                js = json.loads(line.decode('utf-8'))
                cmd = js.get(u'cmd', None)
                if cmd == u'seal':
                    self.sealed.add(js[u'url'])
                else:
                    self.update_memview(parse_vote(js))

    def vote_key(self, vote):
        return (vote.group, vote.hsh)

    def update_memview(self, vote):
        if not vote:
            return
        key = self.vote_key(vote)
        self.votes[key] = vote
        self.sealed.discard(vote.url)

    def write_packet(self, packet):
        with open(self.FILE_NAME, 'a') as outf:
            print >>outf, json.dumps(packet, ensure_ascii=False, encoding='utf-8').encode('utf-8')

    def store(self, vote):
        with self.mutex:
            self.write_packet(vote.to_js_object())
            self.update_memview(vote)

    def vote_counts_for_group(self, group):
        result = {}
        with self.mutex:
            for v in self.votes.itervalues():
                if v.group == group and v.state != STATE_NONE:
                    result[v.url] = result.get(v.url, 0) + 1
        return result

    class TUrlStats(object):
        def __init__(self):
            self.votes = 0
            self.sealed = False

    def stats_by_url(self):
        result = {}
        with self.mutex:
            for v in self.votes.itervalues():
                stat = result.get(v.url, None)
                if stat is None:
                    stat = self.TUrlStats()
                    result[v.url] = stat
                if v.state != STATE_NONE:
                    stat.votes += 1
            for url in self.sealed:
                stat = result.get(url, None)
                if stat is None:
                    stat = self.TUrlStats()
                    result[url] = stat
                stat.sealed = True
        return result

    def get(self, key):
        with self.mutex:
            if key in self.votes:
                return self.votes[key]
            else:
                return None

    def search(self, vote):
        return self.get(self.vote_key(vote))

    def seal(self, url):
        with self.mutex:
            if url in self.sealed:
                return
            self.write_packet({u'cmd': u'seal', u'url' : url})
            self.sealed.add(url)

    def is_sealed(self, url):
        with self.mutex:
            return url in self.sealed

class Url(object):
    __slots__ = ['url', 'group']

def group_urllist(urls):
    group_entries = {}
    url_to_entry = {}
    for group, url in urls:
        entry = Url()
        entry.url = url
        entry.group = group
        if group in group_entries:
            group_entries[group].append(entry)
        else:
            group_entries[group] = [entry]
        url_to_entry[url] = entry
    for group, entries in group_entries.iteritems():
        entries.sort(key = lambda e: e.url)
    return group_entries, url_to_entry


config = {
    'configDir': ".",
    'port': 8044,
    'zoraSource': "RichContentEval",
    'zoraUserproxy': False,
    'zoraTimeout': 30000
}

app = Flask(__name__)
app.config['JSON_AS_ASCII'] = False
app.jinja_env.filters['json'] = escaped_jsonify

cache = Cache()
cache.LoadCache()

fetcher = ZoraFetcher(config['zoraSource'], config['zoraUserproxy'], False, config['zoraTimeout'])

proxy = proxylib.Proxy(fetcher, cache, config['configDir'])

groups, urls = group_urllist(load_urllist())
url_groups_sorted = sorted(groups.iterkeys())

# Voting
STATE_NONE = u"none"
STATE_BAD = u"bad"
STATE_GOOD = u"good"
STATE_TRANSITIONS = {STATE_NONE:STATE_BAD, STATE_BAD:STATE_GOOD, STATE_GOOD:STATE_NONE}

votes = VoteDB()

class GroupItem(object):
    def __init__(self, group):
        self.group = group
        self.votes = 0
        self.touched = 0
        self.sealed = 0

@app.route("/")
def url_groups_handler():
    url_stats = votes.stats_by_url()
    group_stats = dict(( (group, GroupItem(group)) for group in groups ))
    for url, entry in urls.iteritems():
        group_stat = group_stats[entry.group]
        url_stat = url_stats.get(url, None)
        if url_stat is None:
            continue
        if url_stat.votes > 0:
            group_stat.votes += url_stat.votes
            group_stat.touched += 1
        if url_stat.sealed:
            group_stat.sealed += 1

    url_groups = [group_stats[group] for group in url_groups_sorted]
    return render_template('url_groups.html', url_groups=url_groups)


class UrlItem(object):
    __slots__ = ['url', 'votes', 'sealed']

    def __init__(self, url):
        self.url = url
        self.votes = 0
        self.sealed = False

@app.route("/urls")
def url_list_handler():
    group = request.args.get('group', '').encode('utf-8')
    if group not in groups:
        return 'URL group not found', 404
    group_urls = groups[group]
    url_stats = votes.stats_by_url()
    items = []
    for url in group_urls:
        item = UrlItem(url.url)
        url_stat = url_stats.get(url.url)
        if url_stat is not None:
            item.votes = url_stat.votes
            item.sealed = url_stat.sealed
        items.append(item)

    return render_template('url_list.html', urls=items, group=group)

class Sent(object):
    freq = None
    text = None
    state = None

    def __init__(self, ratio, text, state):
        self.state = state.encode('utf-8')
        self.freq = int(ratio*100)
        if len(text) > 100:
            self.text = text[:40] + "..." + text[-40:]
        else:
            self.text = text
        if ratio <= 0.03:
            self.color = 'white'
        elif ratio < 0.80:
            self.color = '#E0B0B0'
        else:
            self.color = '#E00000'

@app.route("/view")
def url_view_handler():
    url = request.args.get('url', '').encode('utf-8')
    if url not in urls:
        return 'URL not found', 404

    url_entry = urls[url]
    url_cache = cache.QueryMetadata(url)
    if url_cache is None:
        return 'URL not in cache', 404 # TODO: force prefetch via Zora

    sents_req_str = 'http://localhost:45011/' + url_cache.contentHash + '?' + urllib.urlencode({'encoding' : url_cache.encoding, 'url' : url})
    print >>sys.stderr, sents_req_str
    url_sents_req = urllib2.urlopen(sents_req_str)
    url_sents = url_sents_req.read()
    url_sents_req.close()
    sents = []
    sentsJson = []
    for line in StringIO(url_sents):
        line = line.strip()
        if not line:
            continue
        freq, hsh, text = line.split('\t', 2)
        state = STATE_NONE
        vote = votes.get((url_entry.group.decode('utf-8'), hsh.decode('utf-8')))
        if vote:
            state = vote.state.encode('utf-8')
        sents.append(Sent(float(freq), text.decode('utf-8'), state))
        sentsJson.append({'text':text.decode('utf-8'),'freq':freq,'hash':hsh})

    anonymized_url = 'http://h.yandex.net/?' + urllib.quote_plus(url)
    is_sealed = votes.is_sealed(url.decode('utf-8'))
    return render_template('url_view.html',
            sents=sents,
            sentsJson=sentsJson,
            url=url,
            anonymized_url=anonymized_url,
            url_group=url_entry.group,
            is_sealed=is_sealed)

@app.route("/fetch")
def url_fetch_handler():
        def getint(name, default):
            return int(request.args.get(name, str(default)))

        url = request.args.get('url', None).encode('utf-8')
        codingHint = getint('coding', -1)
        mimeHint = request.args.get('mime', '').encode('utf-8')
        original = getint('original', 0) == 1
        offline = getint('offline', 0) == 1
        refresh = getint('refresh', 0) == 1

        if not url:
            od = OutputDoc()
            od.MakeError('Not found. Try /fetch?url=http://...', 404)
        else:
            mapper = proxylib.UrlMapper(refresh, offline, codingHint)
            od = proxy.GetUrl(url, mapper, codingHint=codingHint, mimeHint=mimeHint, original=original, refresh=refresh)

        status = od.httpCode
        headers = {
            "Content-Type" : od.MakeContentType(),
            "Content-Security-Policy" : "default-src 'self'; img-src 'self'; script-src 'none'; object-src 'none'; style-src 'self' 'unsafe-inline'",
            "X-Content-Security-Policy" : "default-src 'self'; img-src 'self'; script-src 'none'; object-src 'none'; style-src 'self' 'unsafe-inline'",
            "X-WebKit-CSP" : "default-src 'self'; img-src 'self'; script-src 'none'; object-src 'none'; style-src 'self' 'unsafe-inline'"
        }
        content = od.content
        return (content, status, headers)

@app.route('/vote', methods=['POST'])
def vote():
    def state_error(curr_state, from_state, to_state):
        fmt = '{"error":"Requested transition from %s to %s, but the current state is %s"}'
        return fmt % (from_state, to_state, curr_state)

    reqdata = request.get_json(force=True)
    vote = parse_vote(reqdata)
    oldstate = reqdata.get(u'oldstate', None)

    if vote is None or oldstate is None:
        return '{"error":"Bad JSON format"}'

    conflicted = False
    newstate = vote.state
    storedVote = votes.search(vote)
    currstate = STATE_NONE
    if storedVote:
        currstate = storedVote.state
    if newstate != currstate:
        if oldstate != currstate:
            conflicted = True

    if not conflicted:
        votes.store(vote)
        return '{"state":"%s"}' % vote.state.encode('utf-8')
    else:
        return '{"state":"%s", "conflict":true}' % currstate

@app.route('/seal', methods=['POST'])
def seal():
    reqdata = request.get_json(force=True)
    url = reqdata.get(u'url', None)
    if url is None:
        return '{"error":"URL is empty"}'
    votes.seal(url)
    return '{}'

if __name__ == "__main__":
    import logging
    err_handler = logging.StreamHandler(sys.stderr)
    err_handler.setLevel(logging.WARNING)
    app.logger.addHandler(err_handler)
    app.run(host="scrooge.search.yandex.net", port=45040, debug=True, use_reloader=False, threaded=True)


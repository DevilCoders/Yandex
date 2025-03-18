#!/usr/bin/env python
from flask import Flask, current_app, render_template, abort, redirect, url_for, request
import os
import os.path
import urllib
import random
import cgi
import sys
import hashlib
import threading
import itertools

SHUFFLE_KEY = '05.11.2013'
MAX_HILITE = 500

app = Flask('maincontenteval')

def parse_mark(mark):
    res = int(mark)
    if res <= 0:
        res = -1 - res
    return res

def pack_mark(mark):
    if mark <= 0:
        mark = -1 - mark
    return str(mark)

store_mutex = threading.Lock()
def store_mark(marks, url, mark, comment):
    print >>sys.stderr, 'Mark:', mark, url
    store_mutex.acquire()
    try:
        with open('marks', 'ab') as outf:
            outf.write('%s\t%s\t%s\n' % (url, pack_mark(mark), comment))
            outf.flush()
            os.fsync(outf.fileno())
        marks[url] = (mark, comment)
    finally:
        store_mutex.release()

def sort_key(url):
    hsh = hashlib.md5()
    hsh.update(url)
    hsh.update(SHUFFLE_KEY)
    return hsh.digest()

def load_diffs():
    diffs = []
    with open('maincontentchanges', 'rb') as inf:
        for line in inf:
            if not line.strip():
                continue
            url, old, new = (part.strip() for part in line.split('\t'))
            diffs.append((url, old, new))
    diffs.sort(key = lambda diff:sort_key(diff[0]))
    return diffs

def load_marks():
    marks = {}
    if not os.path.isfile('marks'):
        return marks
    with open('marks', 'rb') as inf:
        for line in inf:
            if not line.strip():
                continue
            url, mark, comment = (part.strip() for part in line.split('\t'))
            mark = parse_mark(mark)
            marks[url] = (mark, comment)
    return marks

def next_doc(doc, docs, marks, unmarked_only):
    n = len(docs)
    while n > 0:
        doc += 1
        if (doc > len(docs)):
            doc = 0
        if not unmarked_only or docs[doc][0] not in marks:
            break
    url = url_for('assess', doc=doc)
    if not unmarked_only:
        url += '?unmarked_only=0'
    return redirect(url)

@app.route("/")
def index():
    return next_doc(-1, current_app.diffs, current_app.marks, True)

def renderable_items(old, new):
    def split_numbers(sents, source_index):
        result = []
        for sent in sents:
            if not sent:
                continue
            sent_num, sent = sent.split(' ', 1)
            result.append((int(sent_num), source_index, sent))
        return result

    all_items = []
    result = []
    sources = (old, new)
    num_sources = len(sources)
    empty_item = [None] * num_sources
    hilited_lens = [MAX_HILITE] * num_sources
    first_indices = [None] * num_sources

    for i, source in enumerate(sources):
        all_items += split_numbers(source.decode('utf-8', errors='replace').split(u'\r'), i)
    all_items.sort(key=lambda item:item[0])
    for sent_num, items in itertools.groupby(all_items, key=lambda item:item[0]):
        result_item = [sent_num] + empty_item
        for item in items:
            source_idx = item[1]
            sent = item[2]
            if first_indices[source_idx] is None:
                first_indices[source_idx] = len(result)
            do_hilite = hilited_lens[source_idx] > 0
            hilited_lens[source_idx] -= len(sent)
            sent = cgi.escape(sent)
            if do_hilite:
                sent = '<span class="hilited">' + sent + '</span>'
            result_item[source_idx + 1] = sent
        result.append(result_item)

    return (result, first_indices)


@app.route("/assess/<int:doc>", methods=['GET', 'POST'])
def assess(doc):
    diffs = current_app.diffs
    marks = current_app.marks
    if len(diffs) <= doc or doc < 0:
        print >>sys.stderr, len(diffs), doc
        abort(404)
    url, old, new = diffs[doc]

    if request.method == 'GET':
        unmarked_only = request.args.get('unmarked_only', '1') == '1'
        if url in marks:
            marked_id = marks[url][0]
        else:
            marked_id = -2
        print >>sys.stderr, 'Marked ID', marked_id, url 
        items, first_indices = renderable_items(old, new)

        switch = random.random() >= 0.5
        if switch:
            variants = [1, 0]
        else:
            variants = [0, 1]

        hidden_url = 'http://h.yandex.net/?' + urllib.quote(url)
        return render_template('assess.html', \
                url = url.decode('utf-8', errors='replace'), \
                hidden_url = hidden_url.decode('utf-8', errors='replace'), \
                items = items,
                first_indices = first_indices,
                variants = variants,
                current_mark = marked_id,
                unmarked_only = unmarked_only)
    elif request.method == 'POST':
        mark = request.form['mark']
        unmarked_only = request.form.get('unmarked_only', '0') == '1'

        if not mark:
            mark = None
        else:
            try:
                mark = int(mark)
            except ValueError:
                mark = None

        if mark != None and mark >= -1 and mark <= 1:
            store_mark(marks, url, mark, '')
        return next_doc(doc, diffs, marks, unmarked_only)

if __name__ == "__main__":
    app.diffs = load_diffs()
    app.marks = load_marks()
    app.run(host="0.0.0.0", port=14400, debug=True)

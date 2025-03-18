#!/usr/bin/env python
# coding: utf-8

import cgi
import os
import urllib

HOME = 'https://ossa2.yandex.ru/kernel-seinfo/seinfo_viewer.py?'


def ellipsify(text):
    return text[:20] + '...' + text[-20:] if len(text) > 40 else text

def quote(text):
    return cgi.escape(urllib.quote(text, safe = ''))

def compose_mail(url, engine, search_type, query, flags, revision, request):
    body = '''Hi,

kernel/seinfo (built from revision %s) is mistreating the URL %s as:
Engine: %s
Search Type: %s
Query: %s
Flags: %s

See: %s

(Please delete this line and describe the expected results.)
''' % (revision, url, engine, search_type, query, flags, request)

    return body

BINARY = '../seinfo'

def compose_mailhref(mailsubject, mailbody):
    MAILHREF = 'mailto:smikler@yandex-team.ru?cc=kartynnik@yandex-team.ru&subject=$MAILSUBJECT&body=$MAILBODY'

    return MAILHREF                                       \
        .replace('$MAILSUBJECT', quote(mailsubject))      \
        .replace('$MAILBODY', quote(mailbody))


def process(fields):
    if "url" in fields:
        url = fields["url"].value

        print 'Content-Type: text/html; charset=utf-8\n\n'
        print open('index.html').read().replace('value=""', 'value="%s"' % cgi.escape(url))

        from subprocess import Popen, PIPE, STDOUT
        p = Popen([BINARY], stdout = PIPE, stdin = PIPE, stderr = STDOUT)
        out = p.communicate(input = url)[0]

        from version import version
        _, revision = version()
        request = HOME + os.getenv('QUERY_STRING')

        mailsubject = '[kernel/seinfo BRAK] %s' % ellipsify(url)

        if out.rstrip() == '-':
            mailbody = '''Hi,

kernel/seinfo (built from revision %s) is unable to find a search engine in the URL %s.
See: %s

(Please delete this line and describe the expected results.)''' % (revision, url, request)

            print open('../bad.tmpl').read()                      \
                .replace('$REVISION', revision)                   \
                .replace('$MAILHREF', compose_mailhref(mailsubject, mailbody))
        else:
            engine, search_type, query, flags = out.rstrip().split('\t')

            mailbody = compose_mail(url, engine, search_type, query, flags, revision, request)

            print open('../result.tmpl').read()                                \
                .replace('$ENGINE', cgi.escape(engine))                        \
                .replace('$SEARCH_TYPE', cgi.escape(search_type))              \
                .replace('$QUERY', cgi.escape(query))                          \
                .replace('$FLAGS', cgi.escape(flags))                          \
                .replace('$MAILHREF', compose_mailhref(mailsubject, mailbody)) \
                .replace('$REVISION', revision)
            
    else:
        print 'Location: index.html\n\n'

if __name__ == '__main__':
    try:
        fs = cgi.FieldStorage()
        
        process(fs)

    except:
        import traceback
        print 'Content-Type: text/plain\n\n'
        print traceback.format_exc()

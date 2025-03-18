#!/usr/bin/env python
# coding: utf-8

BINARY = '../seinfo'

def version():
    from subprocess import Popen, PIPE, STDOUT

    v = Popen([BINARY, '--version'], stdout = PIPE, stdin = PIPE, stderr = STDOUT)
    version_info = '(Unknown)'
    revision = 'unknown'
    try:
        import re
        version_info = v.communicate()[0]
        revision = re.search('Last Changed Rev:\s+(\d+)', version_info).group(1)
    except:
        pass

    return version_info, revision

if __name__ == '__main__':
    print 'Content-Type: text/html; charset=utf-8\n\n'
    print open('../ver.tmpl').read()                      \
        .replace('$VERSIONINFO', version()[0].replace('\n', '<br />'))

